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

#include "tabletop_importer.h"

#include "core/error_list.h"
#include "core/error_macros.h"
#include "core/io/config_file.h"
#include "core/io/resource_importer.h"
#include "core/os/dir_access.h"
#include "core/os/os.h"
#include "core/resource.h"
#include "core/variant_parser.h"
#include "editor/import/editor_import_collada.h"
#include "editor/import/resource_importer_obj.h"
#include "editor/import/resource_importer_scene.h"
#include "editor/import/resource_importer_texture.h"
#include "editor/import/resource_importer_wav.h"
#include "modules/gltf/editor_scene_importer_gltf.h"
#include "modules/minimp3/resource_importer_mp3.h"
#include "modules/stb_vorbis/resource_importer_ogg_vorbis.h"

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
    
        Ref<EditorSceneImporterCollada> collada_importer;
        collada_importer.instance();
        scene_importer->add_importer(collada_importer);

        Ref<EditorSceneImporterGLTF> gltf_importer;
        gltf_importer.instance();
        scene_importer->add_importer(gltf_importer);

        Ref<EditorOBJImporter> obj_importer;
        obj_importer.instance();
        scene_importer->add_importer(obj_importer);
    }

    if (ResourceFormatImporter::get_singleton()->get_importer_by_name("wav").is_null()) {
        Ref<ResourceImporterWAV> wav_importer;
        wav_importer.instance();
        ResourceFormatImporter::get_singleton()->add_importer(wav_importer);
    }

    if (ResourceFormatImporter::get_singleton()->get_importer_by_name("ogg_vorbis").is_null()) {
        Ref<ResourceImporterOGGVorbis> ogg_importer;
        ogg_importer.instance();
        ResourceFormatImporter::get_singleton()->add_importer(ogg_importer);
    }

    if (ResourceFormatImporter::get_singleton()->get_importer_by_name("mp3").is_null()) {
        Ref<ResourceImporterMP3> mp3_importer;
        mp3_importer.instance();
        ResourceFormatImporter::get_singleton()->add_importer(mp3_importer);
    }
}

TabletopImporter::~TabletopImporter() {}

Error TabletopImporter::import(const String &p_path, const String &p_import_path, const Dictionary &p_options) {
    // Reference: EditorFileSystem::_reimport_file
    
    ERR_FAIL_COND_V(!FileAccess::exists(p_path), ERR_FILE_NOT_FOUND);
    
    DirAccessRef da = DirAccess::create(DirAccess::ACCESS_USERDATA);
    ERR_FAIL_COND_V(!da->dir_exists(p_import_path.get_base_dir()), ERR_FILE_BAD_PATH);

    Map<StringName, Variant> import_params;
    String importer_name;

    // Load import parameters from the filesystem if they exist.
    if (FileAccess::exists(p_path + ".import")) {
        Ref<ConfigFile> cf;
        cf.instance();
        Error err = cf->load(p_path + ".import");
        if (err == OK) {
            if (cf->has_section("params")) {
                List<String> sk;
                cf->get_section_keys("params", &sk);
                for (List<String>::Element *E = sk.front(); E; E = E->next()) {
                    import_params[E->get()] = cf->get_value("params", E->get());
                }
            }

            if (cf->has_section("remap")) {
                importer_name = cf->get_value("remap", "importer");
            }
        }
    }

    if (importer_name == "keep") {
        return OK;
    }

    Ref<ResourceImporter> importer;
    if (importer_name != "") {
        importer = ResourceFormatImporter::get_singleton()->get_importer_by_name(importer_name);
    }

    if (importer.is_null()) {
        // Can't get importer by name, use the file extension instead.
        importer = ResourceFormatImporter::get_singleton()->get_importer_by_extension(p_path.get_extension());
        ERR_FAIL_COND_V(importer.is_null(), ERR_FILE_UNRECOGNIZED);
    }

    List<ResourceImporter::ImportOption> import_options;
    importer->get_import_options(&import_options);

    for (List<ResourceImporter::ImportOption>::Element *E = import_options.front(); E; E = E->next()) {
        String option_name = E->get().option.name;
        Variant option_default = E->get().default_value;

        // Dictionary has the highest priority.
        if (p_options.has(option_name)) {
            import_params[option_name] = p_options.get(option_name, option_default);
        } else if (!import_params.has(option_name)) {
            import_params[option_name] = option_default;
        }
    }

    List<String> import_variants;
    List<String> gen_files;
    Variant metadata;
    Error import_error = importer->import(p_path, p_import_path, import_params, &import_variants, &gen_files, &metadata);
    ERR_FAIL_COND_V(import_error != OK, import_error);

    FileAccess *import_file = FileAccess::open(p_path + ".import", FileAccess::WRITE);
    ERR_FAIL_COND_V(!import_file, ERR_FILE_CANT_WRITE);

    import_file->store_line("[remap]");
    import_file->store_line("");
    import_file->store_line("importer=\"" + importer->get_importer_name() + "\"");
    if (importer->get_resource_type() != "") {
        import_file->store_line("type=\"" + importer->get_resource_type() + "\"");
    }

    Vector<String> dest_paths;
    if (importer->get_save_extension() == "") {
        // No path.
    } else if (import_variants.size()) {
        for (List<String>::Element *E = import_variants.front(); E; E = E->next()) {
            String path = p_import_path.c_escape() + "." + E->get() + "." + importer->get_save_extension();

            import_file->store_line("path." + E->get() + "=\"" + path + "\"");
            dest_paths.push_back(path);
        }
    } else {
        String path = p_import_path + "." + importer->get_save_extension();
        import_file->store_line("path=\"" + path + "\"");
    }

    if (metadata != Variant()) {
        import_file->store_line("metadata=" + metadata.get_construct_string());
    }

    import_file->store_line("");
    import_file->store_line("[deps]\n");

    if (gen_files.size()) {
        Array genf;
        for (List<String>::Element *E = gen_files.front(); E; E = E->next()) {
            genf.push_back(E->get());
            dest_paths.push_back(E->get());
        }

        String value;
        VariantWriter::write_to_string(genf, value);
        import_file->store_line("files=" + value);
        import_file->store_line("");
    }

    import_file->store_line("source_file=" + Variant(p_path).get_construct_string());

    if (dest_paths.size()) {
        Array dp;
        for (int i = 0; i < dest_paths.size(); i++) {
            dp.push_back(dest_paths[i]);
        }
        import_file->store_line("dest_files=" + Variant(dp).get_construct_string() + "\n");
    }

    import_file->store_line("[params]");
    import_file->store_line("");

    for (List<ResourceImporter::ImportOption>::Element *E = import_options.front(); E; E = E->next()) {
        String base = E->get().option.name;
        String value;
        VariantWriter::write_to_string(import_params[base], value);
        import_file->store_line(base + "=" + value);
    }

    import_file->close();
    memdelete(import_file);

    // Storing MD5s is up to the user.

    return OK;
}

void TabletopImporter::_bind_methods() {
    ClassDB::bind_method(D_METHOD("import", "path", "import_path", "options"), &TabletopImporter::import, DEFVAL(Dictionary()));
}
