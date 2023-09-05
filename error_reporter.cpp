/*
    tabletop_club_godot_module
    Copyright (c) 2020-2023 Benjamin 'drwhut' Beddows

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "error_reporter.h"

ErrorReporter::ErrorReporter() {
    eh.errfunc = _error_handler;
    eh.userdata = this;
    add_error_handler(&eh);
}

ErrorReporter::~ErrorReporter() {
    remove_error_handler(&eh);
}

void ErrorReporter::_bind_methods() {
    ADD_SIGNAL(MethodInfo("error_received", PropertyInfo(Variant::STRING, "func"), PropertyInfo(Variant::STRING, "file"), PropertyInfo(Variant::INT, "line"), PropertyInfo(Variant::STRING, "error"), PropertyInfo(Variant::STRING, "errorexp")));
    ADD_SIGNAL(MethodInfo("warning_received", PropertyInfo(Variant::STRING, "func"), PropertyInfo(Variant::STRING, "file"), PropertyInfo(Variant::INT, "line"), PropertyInfo(Variant::STRING, "error"), PropertyInfo(Variant::STRING, "errorexp")));
}

void ErrorReporter::_error_handler(void *p_self, const char *p_func, const char *p_file, int p_line, const char *p_error, const char *p_errorexp, ErrorHandlerType p_type) {
    ErrorReporter *self = (ErrorReporter *)p_self;

    String func_str = String::utf8(p_func);
    String file_str = String::utf8(p_file);
    String error_str = String::utf8(p_error);
    String errorexp_str = String::utf8(p_errorexp);

    if (p_type == ERR_HANDLER_WARNING) {
        self->emit_signal("warning_received", func_str, file_str, p_line, error_str, errorexp_str);
    } else {
        self->emit_signal("error_received", func_str, file_str, p_line, error_str, errorexp_str);
    }
}
