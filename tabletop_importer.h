/*
    tabletop_club_godot_module
    Copyright (c) 2020 Benjamin 'drwhut' Beddows

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

#ifndef TABLETOP_IMPORTER_H
#define TABLETOP_IMPORTER_H

#include "core/error_list.h"
#include "core/reference.h"
#include "core/io/resource_importer.h"
#include "core/os/dir_access.h"

class TabletopImporter : public Reference {
    GDCLASS(TabletopImporter, Reference);

public:
    TabletopImporter();
    ~TabletopImporter();

    Error copy_file(const String &p_from, const String &p_to);
    Error import(const String &p_path);

protected:
    static void _bind_methods();
    static Error _create_import_dir(DirAccess **dir);
    static Error _import_resource(Ref<ResourceImporter> p_importer, const String &p_path);
};

#endif // TABLETOP_IMPORTER_H