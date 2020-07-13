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

#include "core/os/os.h"
#include "editor/import/editor_scene_importer_gltf.h"
#include "editor/import/resource_importer_scene.h"
#include "editor/import/resource_importer_texture.h"

TabletopImporter::TabletopImporter() {
    if (!ResourceImporterTexture::get_singleton()) {
        Ref<ResourceImporterTexture> texture_importer;
        texture_importer.instance();
        ResourceFormatImporter::get_singleton()->add_importer(texture_importer);
    }

    if (!ResourceImporterScene::get_singleton()) {
        Ref<ResourceImporterScene> scene_importer;
        scene_importer.instance();
        ResourceFormatImporter::get_singleton()->add_importer(scene_importer);
    
        Ref<EditorSceneImporterGLTF> gltf_importer;
        gltf_importer.instance();
        scene_importer->add_importer(gltf_importer);
    }
}

TabletopImporter::~TabletopImporter() {}

Error TabletopImporter::copy_file(const String &p_from, const String &p_to) {

    if (!FileAccess::exists(p_from)) {
        return Error::ERR_FILE_NOT_FOUND;
    }

    DirAccess *dir;
    Error import_dir_error = _create_import_dir(&dir);
    if (!(import_dir_error == Error::OK || import_dir_error == Error::ERR_ALREADY_EXISTS)) {
        return import_dir_error;
    }

    // Check to see if the corresponding .md5 file exists, and if it does, does
    // it contain the same hash? If so, we don't need to copy the file again.
    String file_import_name = p_from.get_file() + "-" + p_from.md5_text();
    String md5_file_path = dir->get_current_dir() + "/" + file_import_name + ".md5";

    String md5 = FileAccess::get_md5(p_from);
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
            return Error::ERR_ALREADY_EXISTS;
        }
    }

    memdelete(dir);

    // If either the .md5 file doesn't exist, or the hash is not the same, then
    // copy the file over.
    DirAccess *main_dir = DirAccess::create(DirAccess::AccessType::ACCESS_FILESYSTEM);
    Error copy_error = main_dir->copy(p_from, p_to);

    if (copy_error != Error::OK) {
        return copy_error;
    }

    memdelete(main_dir);
    
    // Finally, create the .md5 file, and store the hash of the file in it.
    md5_file = FileAccess::open(md5_file_path, FileAccess::WRITE);
    if (!md5_file) {
        return Error::ERR_FILE_CANT_WRITE;
    }

    md5_file->store_line(md5);
    md5_file->close();

    memdelete(md5_file);

    return Error::OK;
}

Error TabletopImporter::import_scene(const String &p_path) {

    return _import_resource(ResourceImporterScene::get_singleton(), p_path);
}

Error TabletopImporter::import_texture(const String &p_path) {

    return _import_resource(ResourceImporterTexture::get_singleton(), p_path);
}

void TabletopImporter::_bind_methods() {
    ClassDB::bind_method(D_METHOD("copy_file", "from", "to"), &TabletopImporter::copy_file);
    ClassDB::bind_method(D_METHOD("import_scene", "path"), &TabletopImporter::import_scene);
    ClassDB::bind_method(D_METHOD("import_texture", "path"), &TabletopImporter::import_texture);
}

Error TabletopImporter::_create_import_dir(DirAccess **dir) {
    Error dir_error = Error::OK;
    DirAccess *import_dir = DirAccess::open(OS::get_singleton()->get_user_data_dir(), &dir_error);

    if (dir_error != Error::OK) {
        return dir_error;
    }

    dir_error = import_dir->make_dir(".import");

    if (dir) {
        import_dir->change_dir(".import");
        *dir = import_dir;
    } else {
        memdelete(import_dir);
    }

    return dir_error;
}

Error TabletopImporter::_import_resource(ResourceImporter *p_importer, const String &p_path) {

    if (!FileAccess::exists(p_path)) {
        return Error::ERR_FILE_NOT_FOUND;
    }

    /**
     * STEP 1: Make sure the directories we want to write to exist.
     * 
     * user://.import for the .stex and .md5 files.
     */
    DirAccess *dir;
    Error import_dir_error = _create_import_dir(&dir);
    if (!(import_dir_error == Error::OK || import_dir_error == Error::ERR_ALREADY_EXISTS)) {
        return import_dir_error;
    }

    /**
     * STEP 2: Use the importer object to create a .stex file in the .import
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
    String file_import_path = dir->get_current_dir() + "/" + p_path.get_file() + "-" + p_path.md5_text();
    memdelete(dir);
    
    List<String> import_variants;
    Error import_error = p_importer->import(p_path, file_import_path, params, &import_variants);

    if (import_error != Error::OK) {
        return import_error;
    }

    /**
     * STEP 3: Create a .import file next to the resource file so Godot knows
     * how to load it.
     */

    FileAccess *file = FileAccess::open(p_path + ".import", FileAccess::WRITE);
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

    return Error::OK;
}
