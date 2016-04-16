/*
 Copyright (C) 2010-2016 Kristian Duske
 
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

#ifndef TextCtrlOutputAdapter_h
#define TextCtrlOutputAdapter_h

#include "StringUtils.h"

#include <wx/event.h>
#include <wx/string.h>

#include <iostream>

class wxTextCtrl;

namespace TrenchBroom {
    namespace View {
        class TextCtrlOutputAdapter {
        private:
            wxTextCtrl* m_textCtrl;
        public:
            TextCtrlOutputAdapter(wxTextCtrl* textCtrl);
            ~TextCtrlOutputAdapter();

            template <typename T>
            TextCtrlOutputAdapter& operator<<(const T& t) {
                return append(t);
            }
            
            template <typename T>
            TextCtrlOutputAdapter& append(const T& t) {
                wxString str;
                str << t;
                sendAppendEvent(str);
                return *this;
            }
        private:
            void sendAppendEvent(const wxString& str);
            void OnAsyncAppend(wxThreadEvent& event);
        };
    }
}

#endif /* TextCtrlOutputAdapter_h */
