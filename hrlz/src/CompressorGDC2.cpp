#include "hrlz/CompressorGDC2.hpp"
#include "hrlz/file.hpp"
#include "gdc2/gdc2.hpp"
#include <sys/wait.h>
#include<queue>

namespace Compressors {
    /**
     * Deletes the GDC2-compressed files associated to this output path
    */
    void CompressorGDC2::gdc2_delete_file(const std::string& path) {
        //There are three files to delete: .gdc2_desc, .gdc2_rc, .gdc2_ref
        std::string desc = path + ".gdc2_desc";
        std::string rc = path + ".gdc2_rc";
        std::string ref = path + ".gdc2_ref";
        if (!std::filesystem::remove(desc)) {
            std::cerr << "Attempted to remove " << desc << " but wasn't found" << std::endl;
        }
        if (!std::filesystem::remove(rc)) {
            std::cerr << "Attempted to remove " << rc << " but wasn't found" << std::endl;
        }
        if (!std::filesystem::remove(ref)) {
            std::cerr << "Attempted to remove " << ref << " but wasn't found" << std::endl;
        }
    }

    size_t get_compressed_size(const std::filesystem::path& archive_full_path) {
        std::string path = archive_full_path.string();
        std::string desc = path + ".gdc2_desc";
        std::string rc = path + ".gdc2_rc";
        std::string ref = path + ".gdc2_ref";
        size_t weight = 0;
        weight += filesystem::file_size(desc);
        weight += filesystem::file_size(rc);
        return weight;
    }

    /**
     * PRECONDITION: argv MUST have a null pointer as its last argument.
     * Runs the GDC2 binary with the given arguments.
     * @param argv The arguments for the program
     * @return the status code of the child process when it finishes.
     */
    int exec_gdc2(char *argv[]) {
        std::string gdc2_exec_dir = APP_DIR_STR;
        std::filesystem::path gdc2_exec_path = gdc2_exec_dir;
        gdc2_exec_path /= "gdc2_exec";
        std::cout << "Executing GDC2 at " << gdc2_exec_path << std::endl;
        pid_t pid = fork();
        if (pid == -1) {
            //Fork failed
            std::cerr << "Couldn't make child process for GDC2" << std::endl;
            throw std::runtime_error("Couldn't make child process for GDC2");
        } else if (pid > 0) {
            //We are in the parent process;
            int status;
            //Wait synchronously for the child
            waitpid(pid, &status, 0);
            return status;
        } else {
            //We are in the child process. Execute GDC2 with the given arguments
            execvp(gdc2_exec_path.c_str(), argv);
            //If the execl function worked, then this line will never be executed. If we are here, there's an error
            std::cerr << "Error calling gdc2 at path " << gdc2_exec_path << std::endl;
            throw std::ios_base::failure("GDC2 error");
        }
    }

    std::size_t CompressorGDC2::compress_pair_keep(const std::filesystem::path& ref_path, const std::filesystem::path& target_path) {
        std::size_t weight;
        //First, create the arguments. We have the path to the target file in target_path,
        //and the path to the reference file in the argument ref_path.
        //We want to get their filenames to make the filename of the output (the name of the archive),
        //and then call file::get_output_filename to prepend the output directory to it
        std::string target_filename = target_path.filename();
        std::string reference_filename = ref_path.filename();
        std::string archive_path = filesystem::get_output_filename_gdc2(reference_filename, target_filename);
        //Now, we get them in c-style strings to pass it to the main call.
        //The main call accepts non-const chars. We make a non-const C-style array copy with the helper function
        std::string string_args[] = {"gdc2_exec", "c", archive_path, ref_path, target_path};
        char *argv[6];
        std::unique_ptr<char []> args_alloc = make_nonconst_copy(string_args, argv, 5);
        argv[5] = nullptr;
        //Call GDC2 main
        int res;
        try {
            res = exec_gdc2(argv); //With subprocess
        } catch (std::exception& e) {
            //Handle generic error
            std::cerr << "Error calling gdc2 with arguments:" << std::endl;
            print_args(5, argv);
            std::cerr << std::endl;
            throw e;
        }
        if (res != 0) {
            //Something went wrong
            std::cerr << "GDC2 Finished with bad error code, launched gdc2 with arguments:" << std::endl;
            print_args(5, argv);
            std::cerr << std::endl;
        }
        //Check if the execution gave an output
        std::filesystem::path compressed_target = archive_path + ".gdc2_rc";
        if (!std::filesystem::exists(compressed_target)) {
            std::cerr << "GDC2 did not produce an output, terminating" << std::endl;
            throw std::ios_base::failure("No output from GDC2");
        }
        //Compression may have been successful. We need to know the output file size
        //Unfortunately, the most reliable way to do it is by opening the file and reading it in full,
        //which is terrible (I wish gdc2 would give the info).
        //however, we should have made a small compressed file :)
        std::size_t size = filesystem::file_size(compressed_target);
        weight = size;
        return weight;
    }

    std::size_t CompressorGDC2::compress_pair_keep(const std::filesystem::path& ref_path, const std::filesystem::path& target_path,
                                                   const std::filesystem::path& ref_sa_path) {
        return compress_pair_keep(ref_path, target_path);
    }

    /**
     * Compresses a pair ref-target with GDC2
    */
    std::size_t CompressorGDC2::compress_pair_delete(const std::filesystem::path& ref,
            const std::filesystem::path& file_to_parse) {
        std::size_t weight = compress_pair_keep(ref, file_to_parse);
        //Now, we have written the file to the output and we should delete it.
        //First, we get the output path
        std::string filename_ref = ref.filename();
        std::string filename_target = file_to_parse.filename();
        std::string archive_absolute_path = filesystem::get_output_filename_gdc2(filename_ref, filename_target);
        //Then, we call delete on it
        gdc2_delete_file(archive_absolute_path);
        return weight;
    }

    std::size_t CompressorGDC2::compress_pair_delete(const std::filesystem::path& ref,
                                                     const std::filesystem::path& file_to_parse,
                                                     const std::filesystem::path& ref_sa_path) {
        return compress_pair_delete(ref, file_to_parse);
    }

    void CompressorGDC2::compress_sequence_and_keep(const std::filesystem::path& ref, std::string **targets, std::size_t n) {
        //Prepare the arguments
        //The archive name is just going to be the name of the reference, since there is only one compression with it
        std::string ref_name = ref.filename().string();
        std::string archive_path = output_dir / ref_name;
        //The rest of the paths are just the targets
        std::string *strings = new std::string[n+4];
        strings[0] = "gdc2_exec";
        strings[1] = "c";
        strings[2] = archive_path;
        strings[3] = ref.string();
        for (std::size_t i = 0; i < n; i++) {strings[i+4] = (*targets)[i];}
        char **argv = new char *[n+5];
        std::unique_ptr<char[]> alloc = make_nonconst_copy(strings, argv, n+4);
        argv[n+4] = (char *)0;
        int res = 0;
        try {
            res = exec_gdc2(argv);
        } catch (std::exception& e) {
            std::cerr << "Exception when calling GDC2 with arguments: " << std::endl;
            print_args(n+4, argv);
            throw e;
        }
        if (res != 0) {
            std::cerr << "Bad return code when calling GDC2 with arguments: " << std::endl;
            print_args(n+4, argv);
        }
        delete[] strings;
        delete[] argv;
        //Check that the output is actually there
        std::string filename = ref.filename().string() + ".gdc2_rc";
        std::filesystem::path output = output_dir / filename;
        if (!std::filesystem::exists(output)) {
            std::cerr << "No output for sequence " << ref.filename() << std::endl;
            throw std::ios_base::failure("No output for GDC2");
        }

        //Done, no need to check the file size, and the output is in the right directory
    }

    /**
     * Call decompress with gdc2 on an archive that has the ref_name as its only archive name (apart from the input path).
     * Useful for decompressing sequences.
     * It will leave all the files in their original directory.
    */
    void CompressorGDC2::decompress_sequence(const std::string& ref_name, const std::filesystem::path& input_directory) {
        std::string arg_strings[] = {"gdc2_exec", "d", (input_directory / ref_name).string()};
        char *argv[4];
        std::unique_ptr<char[]> alloc = make_nonconst_copy(arg_strings, argv, 3);
        argv[3] = (char *)0;
        int res = 0;
        try {
            res = gdc2main(3,argv);
        } catch (std::exception& e) {
            std::cerr << "Exception when calling GDC2 with arguments: " << std::endl;
            print_args(3, argv);
            throw e;
        }
        if (res != 0) {
            std::cerr << "Bad return code when calling GDC2 with arguments: " << std::endl;
            print_args(3, argv);
        }
    }

    /**
     * Methods to compress the whole hierarchy with a single call to GDC2
    */

    size_t CompressorGDC2::compress_hrlz(const DynArray<std::filesystem::path>& files,
                                       const std::unique_ptr<std::vector<std::size_t>[]>& arbor, const std::size_t root_id) {
        //compress_hrlz_gdc2_only_children(files, arbor, root_id);
        compress_hrlz_gdc2_full(files, arbor, root_id);
        //Get full size
        //Archive name is just the root filename in the output path
        return get_compressed_size(output_dir / files[root_id].filename());
    }

    size_t CompressorGDC2::compress_dataset(const DynArray<std::filesystem::path>& files,
                                            const std::unique_ptr<std::vector<std::size_t>[]>& arbor, const std::size_t root_id,
                                            const DynArray<std::filesystem::path>& sa_files)  {
        return compress_hrlz(files, arbor, root_id);
    }
    /**
     * Compresses a hierarchy by passing the order of the files to GDC2 and compressing everything
     * with one call to GDC2
    */
    void CompressorGDC2::compress_hrlz_gdc2_full(const DynArray<std::filesystem::path>& file_paths,
			const std::unique_ptr<std::vector<std::size_t>[]>& arbor, const std::size_t root_id) {
        //Create an array with all the file paths in the order specified by the hierarchy
        std::size_t n_sequences = file_paths.size();
        std::string *paths = new std::string[n_sequences + 4];
        paths[0] = "gdc2_exec";
        paths[1] = "c";
	    paths[2] = "-t30";
        //Archive name is just the root filename in the output path
        paths[3] = (output_dir / file_paths[root_id].filename()).string();
        std::vector<std::size_t> *adj = arbor.get();
        //We are going to put the files in order (first is root, then in levels)
        //Assuming that all sequences are reachable from a BFS traversal of the arboricity
        //(as it should be), we will put all file paths in order in the arguments
        std::queue<std::size_t> queue;
        queue.push(root_id);
        std::size_t i_args = 4;
        while (!queue.empty()) {
            std::size_t ref = queue.front();
            queue.pop();
            paths[i_args] = file_paths[ref].string();
            i_args++;
            std::vector<std::size_t> targets = adj[ref];
            for (std::size_t j = 0; j < targets.size(); j++) {
                queue.push(targets[j]);
            }
        }
        //We have all the arguments in place. Call gdc2
        char **argv = new char *[n_sequences + 5];
        std::unique_ptr<char[]> alloc = make_nonconst_copy(paths, argv, n_sequences + 4);
        argv[n_sequences + 4] = (char*)0;
        int res = 0;
        try {
            res = exec_gdc2(argv);
        } catch (std::exception& e) {
            std::cerr << "Exception when calling GDC2 with arguments: " << std::endl;
            print_args(n_sequences + 4, argv);
            throw e;
        }
        if (res != 0) {
            std::cerr << "Bad return code when calling GDC2 with arguments: " << std::endl;
            print_args(n_sequences + 4, argv);
        }
        //We have compressed the database in the right place.
        write_external_metadata(file_paths, arbor, output_dir, root_id);
        delete[] paths;
        delete[] argv;
        //Check if the output is there
        std::string filename_out = file_paths[root_id].filename().string() + ".gdc2_rc";
        if (!std::filesystem::exists(output_dir / filename_out)) {
            std::cerr << "No output for GDC2 compressing with ref " << file_paths[root_id] << std::endl;
            throw std::ios_base::failure("No output for GDC2");
        }
    }

    void CompressorGDC2::decompress_dataset(const std::filesystem::path& input_directory) {
        decompress_hrlz_gdc2_full(input_directory);
    }

    void CompressorGDC2::decompress_hrlz_gdc2_full(const std::filesystem::path& input_directory) {
        std::unique_ptr<ExternalMetadata> met = read_external_metadata(input_directory);
        std::size_t root_id = met->root_id;
        std::string *filenames = met->filenames.get();
        std::string& root_filename = filenames[root_id];
        decompress_sequence(root_filename, input_directory);
        //Check that the output is there
        std::size_t n = met->n_sequences;
        std::filesystem::path original_directory = met->original_directory;
        for (std::size_t i = 0; i < n; i++) {
            std::string out_filename = filenames[i] + ".ori";
            if (!std::filesystem::exists(original_directory / out_filename)) {
                std::cerr << "No output for sequence " << filenames[i] << std::endl;
                throw std::ios_base::failure("No GDC2 output");
            }
        }
        //Now all files have been decompressed into their original directory with the extension ".ori".
        //We should move them to the output directory.
        for (std::size_t i = 0; i < n; i++) {
            std::string out_filename = filenames[i] + ".ori";
            std::filesystem::rename(original_directory / out_filename,
                                    output_dir / filenames[i]);
        }
    }

    /**
     * LEGACY CODE THAT MIGHT BE USEFUL
    */

    /*
    void CompressorGDC2::compress_hrlz_gdc2_only_children(const DynArray<std::filesystem::path>& file_paths,
                                                          const std::unique_ptr<std::vector<std::size_t>[]>& arbor, const int root_id) {
        std::queue<std::size_t> q;
        q.push(root_id);
        //Traverse arboricity starting from the root, in a BFS manner
        while (!q.empty()) {
            std::size_t ref = q.front();
            q.pop();
            std::vector<std::size_t> children = arbor[ref];
            std::size_t n_children = children.size();
            if (n_children > 0) {
                //There are targets to compress
                //We need to pass to compress_sequence_and_keep the list of filenames of the targets
                //It's cumbersome but doable
                std::string *targets = new std::string[n_children];
                for (std::size_t j = 0; j < n_children; j++) {
                    targets[j] = (file_paths[children[j]]).string();
                    q.push(children[j]); //push the target to the queue
                }
                compress_sequence_and_keep(file_paths[ref], &targets, n_children);
                delete[] targets;
            }
        }
        //All the targets have been written. Now we just need to compress the metadata
        write_external_metadata(file_paths, arbor, output_dir, root_id);
    }
     */

    /*
    void CompressorGDC2::decompress_hrlz_gdc2_only_children(const std::filesystem::path& input_directory) {
        //Read the metadata first
        std::unique_ptr<ExternalMetadata> met = read_external_metadata(input_directory);
        //Now, we can decompress everything. We are keeping the references compressed for now, so they should be
        //in the same directory.
        //The sequences that are both targets and references will get decompressed twice, but since they will have
        //the same filename, I think GDC2 will overwrite the first one, and we will get only one decompressed file
        //in the end
        std::size_t root_id = met->root_id;
        std::vector<std::size_t> *arbor = met->arbor.get();
        std::string *filenames = met->filenames.get();
        //BFS traversal
        std::queue<std::size_t> queue;
        queue.push(root_id);
        while (!queue.empty()) {
            std::size_t ref = queue.front();
            queue.pop(); //Why does pop() not return the front element?? STL please
            std::vector<std::size_t> targets = arbor[ref];
            if (targets.size() > 0) {
                //All children are decompressed with the reference, with the reference filename as archive filename
                decompress_sequence(filenames[ref], input_directory);
                for (std::size_t j = 0; j < targets.size(); j++)
                    queue.push(targets[j]);
            } //Else, no children, no work to do
        }
        //Now all files have been decompressed into their original directory with the extension ".ori".
        //We should move them to the output directory.
        int n = met->n_sequences;
        std::filesystem::path original_directory = met->original_directory;
        for (int i = 0; i < n; i++) {
            std::filesystem::rename((original_directory / filenames[i]).concat(".ori"),
                                    output_dir / filenames[i]);
        }
    }
     */

    /**
     * Compress the hierarchy by passing to GDC2 all the children of a node.
     */
    /*
    void compress_hrlz_gdc2(const DynArray<std::filesystem::path>& file_paths,
            const std::vector<Edge>& edgesBFS, const int root_id) {
        //First, go through the hierarchy in BFS order and do the relative compression
        //corresponding to each edge. The first reference is the root.
        std::unique_ptr<CompressorGDC2> comp = std::make_unique<CompressorGDC2>(file_paths[root_id]);
        std::size_t current_ref = root_id;
        for (std::size_t i = 0; i < edgesBFS.size(); i++) {
            Edge edge = edgesBFS[i];
            //The reference might be different from the previous edge's reference.
            //In that case, we have to create a new compressor from the new reference
            if (edge.from != current_ref) {
                comp.reset(new CompressorGDC2(file_paths[edge.from])); //Deletes the old object
                current_ref = edge.from;
            }
            //Compress the target and store in the output directory
            comp->compress_and_keep(file_paths[edge.to]);
            //That's it. To the next one
        }
        //Now, we have compressed all the pairs to disk. We must now store the hierarchy itself
        //so that we can decompress it later
        //This function is in file.cpp
        write_external_metadata(edgesBFS, file_paths, output_dir, root_id);
        //The unique pointer deletes itself
    };
    */

    /**
     * Decompresses with GDC2 the compressed pair
    */
    /*
    void CompressorGDC2::decompress_pair(const std::string& ref_name, const std::string& target_name, 
            const std::filesystem::path& input_directory, const std::filesystem::path& origin_directory) {
        //We gotta call GDC2 with the archive name as argument
        std::string archive_name_string = filesystem::get_filename_gdc2(ref_name, target_name);
        std::filesystem::path archive_path = input_directory / archive_name_string;
        std::string archive_path_string = archive_path.string();
        //Again, copy of the string to a non-const c-style string to pass as argument
        char *archive_c_str = new char[archive_path_string.size() + 1];
        archive_path_string.copy(archive_c_str, archive_path_string.size());
        archive_c_str[archive_path_string.size()] = '\0';
        char executable_name[] = "gdc2";
        char arg_decompression[] = "d";
        char *argv[] = {executable_name, arg_decompression, archive_c_str};
        //Call GDC2 main
        int res;
        try {
            res = gdc2main(3, argv); //No subprocess! :)
        } catch (std::exception& e) {
            //Handle generic error
            std::cerr << "Error calling gdc2 at path " << gdc2_exec_path << " with arguments:" << std::endl;
            std::cerr << "gdc2" << " " << "d" << archive_c_str << std::endl;
            throw std::ios_base::failure("GDC2 error");
        }
        if (res != 0) {
            //An error occurred
            std::cerr << "Error calling gdc2 at path " << gdc2_exec_path << " with arguments:" << std::endl;
            std::cerr << "gdc2" << " " << "d" << archive_c_str << std::endl;
            throw std::ios_base::failure("GDC2 error");
        }
        delete archive_c_str;
        //Now we have decompressed the files in the original directory. We shall move them to the output directory
        //We have the filenames "reference.ori" and "target.ori"
        std::filesystem::path reference = (origin_directory / ref_name).concat(".ori");
        std::filesystem::path target = (origin_directory / target_name).concat(".ori");
        std::filesystem::rename(reference, output_dir / reference.filename());
        std::filesystem::rename(target, output_dir / target.filename());
    }
    */

    /**
     * Decompress the hierarchy specified in this directory with gdc2
    */
    /*
    void decompress_hrlz_gdc2(const std::filesystem::path& input_directory) {
        //First, read the metadata
        std::unique_ptr<ExternalMetadata> met = read_external_metadata(input_directory);
        std::size_t n = met->n_sequences;
        //Now, decompress each pair (in the right directory).
        //The metadata only has the name of the target. For GDC2, we compress the reference
        //and the target together in an archive. We iterate through the BFS order to get
        //both reference and target.
        //There are n-1 edges (n-1 relative compressions)
        for (std::size_t i = 0; i < n-1; i++) {
            std::pair<std::size_t, std::size_t> pair = met->order[i];
            std::size_t ref_id = pair.first; std::size_t target_id = pair.second;
            std::string target_name = met->filenames[target_id];
            std::string ref_name = met->filenames[ref_id];
            CompressorGDC2::decompress_pair(ref_name, target_name, input_directory, met->original_directory);
        }
        //That's it
    }
    */


    /**
 * Execute GDC2 with the given arguments as a subprocess.
    */
    /*
    int exec_gdc2_compress_pair(char *archive_name, char *reference_seq, char *target_seq) {
        std::string gdc2_exec_path = APP_DIR_STR;
        pid_t pid = fork();
        if (pid == -1) {
            //Fork failed
            std::cerr << "Couldn't make child process for GDC2" << std::endl;
            throw std::runtime_error("Couldn't make child process for GDC2");
        } else if (pid > 0) {
            //We are in the parent process;
            int status;
            //Wait synchronously for the child
            waitpid(pid, &status, 0);
            return status;
        } else {
            //We are in the child process. Execute GDC2 with the given arguments
            execl(gdc2_exec_path.c_str(), "gdc2", "c", archive_name, reference_seq, target_seq, (char *)0);
            //If the execl function worked, then this line will never be executed. If we are here, there's an error
            std::cerr << "Error calling gdc2 at path " << gdc2_exec_path << " with arguments:" << std::endl;
            std::cerr << " " << "gdc2" << " " << "c" << archive_name << " " << reference_seq << " " << target_seq << std::endl;
            throw std::ios_base::failure("GDC2 error");
        }
    }
     */
}
