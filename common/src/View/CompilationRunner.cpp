/*
 Copyright (C) 2010-2017 Kristian Duske

 This file is part of TrenchBroom.

 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with TrenchBroom. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CompilationRunner.h"

#include <QDir>
#include <QMetaEnum>
#ifndef __WASM__
#include <QProcess>
#endif
#include <QtGlobal>

#include "Error.h"
#include "Exceptions.h"
#include "IO/DiskIO.h"
#include "IO/ExportOptions.h"
#include "IO/PathInfo.h"
#include "IO/PathMatcher.h"
#include "IO/PathQt.h"
#include "IO/TraversalMode.h"
#include "Model/CompilationProfile.h"
#include "Model/CompilationTask.h"
#include "View/CompilationContext.h"
#include "View/CompilationVariables.h"
#include "View/MapDocument.h"

#include "kdl/functional.h"
#include "kdl/overload.h"
#include "kdl/result_fold.h"
#include "kdl/string_utils.h"
#include "kdl/vector_utils.h"

#include <filesystem>
#include <string>

namespace TrenchBroom::View
{
CompilationTaskRunner::CompilationTaskRunner(CompilationContext& context)
  : m_context{context}
{
}

CompilationTaskRunner::~CompilationTaskRunner() = default;

void CompilationTaskRunner::execute()
{
  doExecute();
}

void CompilationTaskRunner::terminate()
{
  doTerminate();
}

std::string CompilationTaskRunner::interpolate(const std::string& spec)
{
  try
  {
    return m_context.interpolate(spec);
  }
  catch (const Exception& e)
  {
    m_context << "#### Could not interpolate expression '" << QString::fromStdString(spec)
              << "': " << e.what() << "\n";
    throw;
  }
}

CompilationExportMapTaskRunner::CompilationExportMapTaskRunner(
  CompilationContext& context, Model::CompilationExportMap task)
  : CompilationTaskRunner{context}
  , m_task{std::move(task)}
{
}

CompilationExportMapTaskRunner::~CompilationExportMapTaskRunner() = default;

void CompilationExportMapTaskRunner::doExecute()
{
  emit start();

  const auto targetPath = std::filesystem::path{interpolate(m_task.targetSpec)};
  m_context << "#### Exporting map file '" << IO::pathAsQString(targetPath) << "'\n";

  if (!m_context.test())
  {
    IO::Disk::createDirectory(targetPath.parent_path()) | kdl::and_then([&](auto) {
      const auto options = IO::MapExportOptions{targetPath};
      const auto document = m_context.document();
      return document->exportDocumentAs(options);
    }) | kdl::transform([&]() { emit end(); })
      | kdl::transform_error([&](auto e) {
          m_context << "#### Could not export map file '" << IO::pathAsQString(targetPath)
                    << "': " << QString::fromStdString(e.msg) << "\n";
          emit error();
        });
  }
  else
  {
    emit end();
  }
}

void CompilationExportMapTaskRunner::doTerminate() {}

CompilationCopyFilesTaskRunner::CompilationCopyFilesTaskRunner(
  CompilationContext& context, Model::CompilationCopyFiles task)
  : CompilationTaskRunner{context}
  , m_task{std::move(task)}
{
}

CompilationCopyFilesTaskRunner::~CompilationCopyFilesTaskRunner() = default;

void CompilationCopyFilesTaskRunner::doExecute()
{
  emit start();

  const auto sourcePath = std::filesystem::path{interpolate(m_task.sourceSpec)};
  const auto targetPath = std::filesystem::path{interpolate(m_task.targetSpec)};

  const auto sourceDirPath = sourcePath.parent_path();
  const auto sourcePathMatcher = kdl::lift_and(
    IO::makePathInfoPathMatcher({IO::PathInfo::File}),
    IO::makeFilenamePathMatcher(sourcePath.filename().string()));

  IO::Disk::find(sourceDirPath, IO::TraversalMode::Flat, sourcePathMatcher)
    | kdl::and_then([&](const auto& pathsToCopy) {
        const auto pathStrsToCopy = kdl::vec_transform(
          pathsToCopy, [](const auto& path) { return "'" + path.string() + "'"; });

        m_context << "#### Copying to '" << IO::pathAsQString(targetPath)
                  << "/': " << QString::fromStdString(kdl::str_join(pathStrsToCopy, ", "))
                  << "\n";
        if (!m_context.test())
        {
          return IO::Disk::createDirectory(targetPath) | kdl::and_then([&](auto) {
                   return kdl::vec_transform(
                            pathsToCopy,
                            [&](const auto& pathToCopy) {
                              return IO::Disk::copyFile(pathToCopy, targetPath);
                            })
                          | kdl::fold;
                 });
        }
        return Result<void>{};
      })
    | kdl::transform([&]() { emit end(); }) | kdl::transform_error([&](auto e) {
        m_context << "#### Could not copy '" << IO::pathAsQString(sourcePath) << "' to '"
                  << IO::pathAsQString(targetPath)
                  << "': " << QString::fromStdString(e.msg) << "\n";
        emit error();
      });
}

void CompilationCopyFilesTaskRunner::doTerminate() {}

CompilationRenameFileTaskRunner::CompilationRenameFileTaskRunner(
  CompilationContext& context, Model::CompilationRenameFile task)
  : CompilationTaskRunner{context}
  , m_task{std::move(task)}
{
}

CompilationRenameFileTaskRunner::~CompilationRenameFileTaskRunner() = default;

void CompilationRenameFileTaskRunner::doExecute()
{
  emit start();

  const auto sourcePath = std::filesystem::path{interpolate(m_task.sourceSpec)};
  const auto targetPath = std::filesystem::path{interpolate(m_task.targetSpec)};

  m_context << "#### Renaming '" << IO::pathAsQString(sourcePath) << "' to '"
            << IO::pathAsQString(targetPath) << "'\n";
  if (!m_context.test())
  {
    IO::Disk::createDirectory(targetPath.parent_path())
      | kdl::and_then([&](auto) { return IO::Disk::moveFile(sourcePath, targetPath); })
      | kdl::transform([&]() { emit end(); }) | kdl::transform_error([&](auto e) {
          m_context << "#### Could not rename '" << IO::pathAsQString(sourcePath)
                    << "' to '" << IO::pathAsQString(targetPath)
                    << "': " << QString::fromStdString(e.msg) << "\n";
          emit error();
        });
  }
  else
  {
    emit end();
  }
}

void CompilationRenameFileTaskRunner::doTerminate() {}

CompilationDeleteFilesTaskRunner::CompilationDeleteFilesTaskRunner(
  CompilationContext& context, Model::CompilationDeleteFiles task)
  : CompilationTaskRunner{context}
  , m_task{std::move(task)}
{
}

CompilationDeleteFilesTaskRunner::~CompilationDeleteFilesTaskRunner() = default;

void CompilationDeleteFilesTaskRunner::doExecute()
{
  emit start();

  const auto targetPath = std::filesystem::path{interpolate(m_task.targetSpec)};
  const auto targetDirPath = targetPath.parent_path();
  const auto targetPathMatcher = kdl::lift_and(
    IO::makePathInfoPathMatcher({IO::PathInfo::File}),
    IO::makeFilenamePathMatcher(targetPath.filename().string()));

  IO::Disk::find(targetDirPath, IO::TraversalMode::Recursive, targetPathMatcher)
    | kdl::transform([&](const auto& pathsToDelete) {
        const auto pathStrsToDelete = kdl::vec_transform(
          pathsToDelete, [](const auto& path) { return "'" + path.string() + "'"; });
        m_context << "#### Deleting: "
                  << QString::fromStdString(kdl::str_join(pathStrsToDelete, ", "))
                  << "\n";

        if (!m_context.test())
        {
          return kdl::vec_transform(pathsToDelete, IO::Disk::deleteFile) | kdl::fold;
        }
        return Result<std::vector<bool>>{std::vector<bool>{}};
      })
    | kdl::transform([&](auto) { emit end(); }) | kdl::transform_error([&](auto e) {
        m_context << "#### Could not delete '" << IO::pathAsQString(targetPath)
                  << "': " << QString::fromStdString(e.msg) << "\n";
        emit error();
      });
}

void CompilationDeleteFilesTaskRunner::doTerminate() {}

CompilationRunToolTaskRunner::CompilationRunToolTaskRunner(
  CompilationContext& context, Model::CompilationRunTool task)
  : CompilationTaskRunner{context}
  , m_task{std::move(task)}
{
}

CompilationRunToolTaskRunner::~CompilationRunToolTaskRunner() = default;

void CompilationRunToolTaskRunner::doExecute()
{
  startProcess();
}

void CompilationRunToolTaskRunner::doTerminate()
{

}

void CompilationRunToolTaskRunner::startProcess()
{

}

std::string CompilationRunToolTaskRunner::cmd()
{
  const auto toolPath = std::filesystem::path{interpolate(m_task.toolSpec)};
  const auto parameters = interpolate(m_task.parameterSpec);
  if (parameters.empty())
  {
    return "\"" + toolPath.string() + "\"";
  }
  else if (toolPath.empty())
  {
    return "";
  }
  else
  {
    return "\"" + toolPath.string() + "\" " + parameters;
  }
}

void CompilationRunToolTaskRunner::processReadyReadStandardError()
{
}

void CompilationRunToolTaskRunner::processReadyReadStandardOutput()
{
}

CompilationRunner::CompilationRunner(
  CompilationContext context, const Model::CompilationProfile& profile, QObject* parent)
  : QObject{parent}
  , m_context{std::move(context)}
  , m_taskRunners{createTaskRunners(m_context, profile)}
  , m_currentTask{std::end(m_taskRunners)}
{
}

CompilationRunner::~CompilationRunner() = default;

CompilationRunner::TaskRunnerList CompilationRunner::createTaskRunners(
  CompilationContext& context, const Model::CompilationProfile& profile)
{
  auto result = TaskRunnerList{};
  for (const auto& task : profile.tasks)
  {
    std::visit(
      kdl::overload(
        [&](const Model::CompilationExportMap& exportMap) {
          if (exportMap.enabled)
          {
            result.push_back(
              std::make_unique<CompilationExportMapTaskRunner>(context, exportMap));
          }
        },
        [&](const Model::CompilationCopyFiles& copyFiles) {
          if (copyFiles.enabled)
          {
            result.push_back(
              std::make_unique<CompilationCopyFilesTaskRunner>(context, copyFiles));
          }
        },
        [&](const Model::CompilationRenameFile& renameFile) {
          if (renameFile.enabled)
          {
            result.push_back(
              std::make_unique<CompilationRenameFileTaskRunner>(context, renameFile));
          }
        },
        [&](const Model::CompilationDeleteFiles& deleteFiles) {
          if (deleteFiles.enabled)
          {
            result.push_back(
              std::make_unique<CompilationDeleteFilesTaskRunner>(context, deleteFiles));
          }
        },
        [&](const Model::CompilationRunTool& runTool) {
          if (runTool.enabled)
          {
            result.push_back(
              std::make_unique<CompilationRunToolTaskRunner>(context, runTool));
          }
        }),
      task);
  }
  return result;
}

void CompilationRunner::execute()
{
  
}

void CompilationRunner::terminate()
{
 
}

bool CompilationRunner::running() const
{
  return false;
}

void CompilationRunner::bindEvents(CompilationTaskRunner& runner) const
{
  connect(&runner, &CompilationTaskRunner::error, this, &CompilationRunner::taskError);
  connect(&runner, &CompilationTaskRunner::end, this, &CompilationRunner::taskEnd);
}

void CompilationRunner::unbindEvents(CompilationTaskRunner& runner) const
{
  runner.disconnect(this);
}

void CompilationRunner::taskError()
{
  if (running())
  {
    unbindEvents(*m_currentTask->get());
    m_currentTask = std::end(m_taskRunners);
    emit compilationEnded();
  }
}

void CompilationRunner::taskEnd()
{
  if (running())
  {
    unbindEvents(*m_currentTask->get());
    ++m_currentTask;
    if (m_currentTask != std::end(m_taskRunners))
    {
      bindEvents(*m_currentTask->get());
      m_currentTask->get()->execute();
    }
    else
    {
      emit compilationEnded();
    }
  }
}

} // namespace TrenchBroom::View
