/*
    open_tabletop_import_module
    Copyright (c) 2020 drwhut

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

#include "tabletop_importer.h"

#include "core/os/dir_access.h"
#include "core/os/os.h"

TabletopImporter::TabletopImporter() {}

Error TabletopImporter::import_texture(const String &p_path, const String &p_game,
    const String &p_type) {
    
    ResourceImporterTexture texture_importer;
    return _import_resource(&texture_importer, p_path, p_game, p_type);
}

void TabletopImporter::_bind_methods() {
    ClassDB::bind_method(D_METHOD("import_texture", "path", "game", "type"), &TabletopImporter::import_texture);
}

Error TabletopImporter::_import_resource(ResourceImporter *p_importer, const String &p_path, const String &p_game, const String &p_type) {

    if (!FileAccess::exists(p_path)) {
        return Error::ERR_FILE_NOT_FOUND;
    }

    // Make sure the folder structure is consistent, despite the arguments.
    p_game.replace("\\", "/");
    p_game.replace("/", "_");

    p_type.replace("\\", "/");
    p_type.replace("/", "_");

    /**
     * STEP 1: Make sure the directories we want to write to exist.
     * 
     * user://.import for the .stex and .md5 files,
     * user://{p_game}/{p_type} for the resources themselves.
     */

    Error dir_error = Error::OK;
    DirAccess *dir = DirAccess::open(OS::get_singleton()->get_user_data_dir(), &dir_error);

    if (dir_error != Error::OK) {
        return dir_error;
    }

    dir_error = dir->make_dir(".import");
    if (!(dir_error == Error::OK || dir_error == Error::ERR_ALREADY_EXISTS)) {
        return dir_error;
    }

    dir_error = dir->make_dir_recursive(p_game + "/" + p_type);
    if (!(dir_error == Error::OK || dir_error == Error::ERR_ALREADY_EXISTS)) {
        return dir_error;
    }

    /**
     * STEP 2: If the corresponding .md5 file exists in .import/, check the md5
     * hash of the resource and if it is the same, stop now (since there's no
     * point in continuing).
     */

    String file_import_name = p_path.get_file() + "-" + p_path.md5_text();
    dir->change_dir(".import");
    String file_import_path = dir->get_current_dir() + "/" + file_import_name;
    String md5_file_path = file_import_path + ".md5";

    String md5 = FileAccess::get_md5(p_path);
    FileAccess *md5_file;

    if (FileAccess::exists(md5_file_path)) {
        md5_file = FileAccess::open(md5_file_path, FileAccess::READ);
        if (!md5_file) {
            return Error::ERR_FILE_CANT_READ;
        }

        String claimed_md5 = md5_file->get_line();

        md5_file->close();
        memdelete(md5_file);

        if (claimed_md5 == md5) {
            return Error::OK;
        }
    }

    /**
     * STEP 3: Copy the resource file over to user://{p_game}/{p_type}/{file_name}
     */

    dir->change_dir("../" + p_game + "/" + p_type);
    DirAccess *main_dir = DirAccess::create(DirAccess::AccessType::ACCESS_FILESYSTEM);
    main_dir->copy(p_path, dir->get_current_dir() + "/" + p_path.get_file());

    memdelete(main_dir);

    /**
     * STEP 4: Use the importer object to create a .stex file in the .import
     * folder.
     * 
     * This point onwards is based from the code in:
     * editor/editor_file_system.cpp EditorFileSystem::_reimport_file
     */

    // Get the default parameters.
    Map<StringName, Variant> params;

    List<ResourceImporter::ImportOption> opts;
    p_importer->get_import_options(&opts);

    for (List<ResourceImporter::ImportOption>::Element *E = opts.front(); E; E = E->next()) {
        params[E->get().option.name] = E->get().default_value;
    }

    // The location where the .stex file will be located.
    dir->change_dir("../../.import");
    
    List<String> import_variants;
    Error import_error = p_importer->import(p_path, file_import_path, params, &import_variants);

    if (import_error != Error::OK) {
        return import_error;
    }

    /**
     * STEP 5: Store the MD5 of the resource so we can check to see if we need
     * to even do all this.
     */
    
    md5_file = FileAccess::open(md5_file_path, FileAccess::WRITE);
    if (!md5_file) {
        return Error::ERR_FILE_CANT_WRITE;
    }

    md5_file->store_line(md5);
    md5_file->close();
    memdelete(md5_file);

    /**
     * STEP 6: Create a .import file next to the resource file so Godot knows
     * how to load it.
     */

    dir->change_dir("../" + p_game + "/" + p_type);
    FileAccess *file = FileAccess::open(dir->get_current_dir() + "/" + p_path.get_file() + ".import", FileAccess::WRITE);
    if (!file) {
        return Error::ERR_FILE_CANT_WRITE;
    }

    file->store_line("[remap]");
    file->store_line("importer=\"" + p_importer->get_importer_name() + "\"");
    if (p_importer->get_resource_type() != "") {
        file->store_line("type=\"" + p_importer->get_resource_type() + "\"");
    }

    if (p_importer->get_save_extension() == "") {
        // No path.
    } else if (import_variants.size()) {
        // Import with variants.
        for (List<String>::Element *E = import_variants.front(); E; E = E->next()) {
            String path = file_import_path.c_escape() + "." + E->get() + "." + p_importer->get_save_extension();
            file->store_line("path." + E->get() + "=\"" + path + "\"");
        }
    } else {
        String path = file_import_path + "." + p_importer->get_save_extension();
        file->store_line("path=\"" + path + "\"");
    }

    file->close();
    memdelete(file);

    memdelete(dir);

    return Error::OK;
}
