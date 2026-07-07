#include<iostream>
#include<fstream>
#include<algorithm>

#include "rlz/rlz.hpp"
#include "hrlz/file.hpp"
#include "common/Timer.hpp"
#include "hrlz/Compressors.hpp"

#include<iostream>

namespace filesystem {
  std::size_t file_size(const std::filesystem::path& path) {
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
    file.open(path, std::ifstream::binary);
    file.seekg(0, file.end);
    std::size_t res = file.tellg();
    file.close();
    return res;
  }

  //Define the global variable output_dir
  std::filesystem::path output_dir; //empty path

  //Initialize folders if they don't exist
  //We use canonical to store the path because sometimes we want to change directory,
  //and then the paths are not valid anymore (we do this in HiRGC)
  void init_output_folder(const std::filesystem::path& output_folder) {
    if (std::filesystem::exists(output_folder)) {
      if (!std::filesystem::is_directory(output_folder)) {
        std::cerr << "Output folder is not a folder but a file: " << std::endl;
        std::cerr << output_folder << std::endl;
      } else {
          filesystem::output_dir = std::filesystem::canonical(output_folder);
      }
    } else {
      //create output folder (gdc2 will complain and not compress if it doesn't exist)
      std::filesystem::create_directory(output_folder);
      filesystem::output_dir = std::filesystem::canonical(output_folder);
    }
  }

  /**
   * Return the absolute path with the filename of the target compressed wrt ref (depends on the output directory),
   * given the filenames of the reference and the target
  */
  std::string get_output_filename_gdc2(const std::string& filename_ref, const std::string& filename_target) {
    std::filesystem::path path_to_file = output_dir / (filename_ref + "_" + filename_target);
    return path_to_file.string();
  }

  std::string get_filename_gdc2(const std::string& filename_ref, const std::string& filename_target) {
    return filename_ref + "_" + filename_target;
  }
}

DynArray<char> read_file(const std::filesystem::path& path, std::size_t right_padding) {
  std::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
  file.open(path, std::ifstream::binary);
  file.seekg(0, file.end);
  auto length = file.tellg();
  file.seekg(0, file.beg);

  DynArray<char> buf(right_padding + length);
  file.read(buf.get(), length);
  file.close();
  return buf;
}

DynArray<char> read_file(const std::filesystem::path& path) {
  return read_file(path, 0);
}

/**
 * Reads the DNA in a FASTA file that has been preprocessed to not
 * have newlines (except maybe in the first line of the file)
*/
DynArray<char> read_compacted_DNA(const std::filesystem::path& path) {
  std::ifstream file;
  //Only crash if there's an exception
  file.exceptions(std::ifstream::badbit);
  if (!std::filesystem::exists(path)) {
      std::cerr << "ERROR: File not found: " << path << std::endl;
      std::exit(1);
  }
  //Open in binary because we are going to do unformatted input
  file.open(path, std::ifstream::binary);
  if (!file) {
      std::cerr << "IO Error reading file: " << path << std::endl;
      std::exit(1);
  }
  //Get size of the file in bytes
  file.seekg(0, file.end);
  std::size_t length = file.tellg();
  //Go to the start and read the first FASTA definition line if it is there
  file.seekg(0, file.beg);
  char c = file.peek();
  if (c == '>') {
    //There's a FASTA definition line that we are going to ignore
    std::string line;
    std::getline(file, line); //Extracts the end of line
    //The remaining length is smaller
    std::size_t curr_pos = file.tellg();
    length = length - curr_pos;
  }
  //Allocate and read the rest in one go
  DynArray<char> buf(length);
  file.read(buf.get(), length);
  file.close();
  return buf;
}

DynArray<char> read_DNA_from_FASTA(const std::filesystem::path& path) {
  //We open the file and get its size in bytes
  std::ifstream file;
  file.exceptions(std::ifstream::badbit);
  file.open(path, std::ifstream::binary);
  file.seekg(0, file.end);
  std::size_t length = file.tellg();
  //Now, we open the file in text mode
  file.close();
  file.open(path);
  //We skip the first line if it is a FASTA definition line
  char c = file.peek();
  if (c == '>') {
    //We have to ignore the first line
    std::string fasta_line;
    std::getline(file, fasta_line);
    //We are on the second line in the stream. Set the length of the file
    //to the remaining chunk
    std::size_t curr_pos = file.tellg();
    length = length - curr_pos;
  }
  DynArray<char> buf(length);
  //Read char by char, and ignore whitespace and newlines
  char *ptr = buf.get();
  std::size_t i = 0;  
  while (file >> c) {
      *ptr++ = c;
      i++;
  }
  //The DynArray holds a memory buffer that can potentially be larger than the data.
  //For furter functions, it is important to have the size attribute determine
  //the size of the data. free() already knows the size of the buffer
  buf.set_size(i);
  file.close();
  return buf;
}

DynArray<std::filesystem::path> read_input(std::istream& input) {
  std::string line;
  std::getline(input, line);
  std::size_t count = stoi(line);
  DynArray<std::filesystem::path> buf(count);
  for(std::size_t i = 0; i < count; ++i) {
    std::getline(input, line);
    buf[i] = line;
  }
  return buf;
}

DynArray<std::filesystem::path> read_input_file(const std::filesystem::path& path) {
  if (!std::filesystem::exists(path)) {
    std::cerr << "FILE DOESN'T EXIST! File: " << std::endl;
    std::cerr << path << std::endl;
  }
  //Max: If we run the executable hrlz from a different path than the input file, and the paths included are relative,
  //then we will be in trouble, because we read the relative paths explicitly without conversion to our current path.
  //Therefore, we will change directory to the file named by path.
  std::filesystem::current_path(path.parent_path());
  std::ifstream file;
  file.exceptions(std::ifstream::badbit);
  file.open(path, std::ifstream::in);
  auto result = read_input(file);
  file.close();
  return result;
}

/**
 * Decompress a database that's contained in a single file.
*/
void decompress_from_stream(std::istream& stream) {
  //Get the file size.
  stream.seekg(0, std::ios_base::end);
  std::size_t file_length = stream.tellg();
  //Struct for the metadata of the file.
  FileMetadata metadata;
  //The struct FileMetadata is located at the end of the file.
  //Read it directly.
  stream.seekg(file_length - sizeof(struct FileMetadata));
  stream.read((char*) &metadata, sizeof(struct FileMetadata));
  //Create an array for the paths of each sequence.
  DynArray<std::filesystem::path> filepaths(metadata.sequence_count);
  //Create an array for the metadata of each sequence
  DynArray<SequenceMetadata> sequence_metadatas(metadata.sequence_count);
  //From the File Metadata we get where the file paths are in the file
  stream.seekg(metadata.filepath_array_start);
  std::vector<char> path;
  //Read char by char the file paths. They are C-style strings, so we
  //read until a zero char appears; that means we have read a full path
  for(std::size_t i = 0; i < metadata.sequence_count; ++i) {
    char c;
    while(true) {
      stream.get(c);
      path.push_back(c);
      if(c == 0)
        break;
    }
    //Store this on filepaths... but with the correct output folder selected by the user
    std::filesystem::path stored(path.begin());
    std::filesystem::path output_path = filesystem::output_dir / stored.filename();
    filepaths[i] = output_path;
    path.clear();
  }
  //Read all of the metadatas in bulk (why didn't we do this for the paths?? We could have stored their size)
  std::copy_n(std::istreambuf_iterator<char>(stream), metadata.sequence_count * sizeof(struct SequenceMetadata),(char*) sequence_metadatas.begin());
  //Map from index in file to index of the sequence (in the order that the metadata have)
  DynArray<std::size_t> file_index_to_seq_index(metadata.sequence_count);
  for(std::size_t i = 0; i < metadata.sequence_count; ++i) {
    file_index_to_seq_index[sequence_metadatas[i].index_in_file] = i;
  }
  //Go to the start to read the sequences
  stream.seekg(0);
  //Get the index, in the metadata ordering, of the root sequence.
  //Store it as the current parent, that we will have uncompressed.
  std::size_t current_ref = metadata.root_index;
  //Read the first reference (the root), that will always be at the top of the Metadata file
  DynArray<char> ref(metadata.root_size);
  std::copy_n(std::istreambuf_iterator<char>(stream), metadata.root_size, ref.begin());
  //Copy the root to its corresponding file
  std::ofstream output_file;
  output_file.open(filepaths[metadata.root_index]);
  std::copy_n(ref.begin(), metadata.root_size, std::ostreambuf_iterator<char>(output_file));
  output_file.close();
  //For all other sequences... decompress in the right order.
  for(std::size_t i = 1; i < metadata.sequence_count; ++i) {
    std::size_t seq_index = file_index_to_seq_index[i];
    if(sequence_metadatas[seq_index].parent != current_ref) {
      current_ref = sequence_metadatas[seq_index].parent;
      ref = read_file(filepaths[current_ref]);
    }
    output_file.open(filepaths[seq_index]);
    for(std::size_t f = 0; f < sequence_metadatas[seq_index].factors; ++f) {
      filelength_type pos, len;
      stream.read((char*) &pos, sizeof(pos));
      stream.read((char*) &len, sizeof(len));
      if(len == 0) {
        output_file.put(pos);
        continue;
      }
      std::copy_n(ref.begin() + pos, len, std::ostreambuf_iterator<char>(output_file));
    }
    output_file.close();
  }
}

void decompress_sequence_from_stream(std::istream& stream, std::size_t seq_index) {
  stream.seekg(0, std::ios_base::end);
  std::size_t file_length = stream.tellg();
  FileMetadata metadata;
  SequenceMetadata seq_metadata;
  stream.seekg(file_length - sizeof(struct FileMetadata));
  stream.read((char*) &metadata, sizeof(struct FileMetadata));

  stream.seekg(file_length - sizeof(struct FileMetadata) - (metadata.sequence_count - seq_index) * sizeof(struct SequenceMetadata));
  stream.read((char*) &seq_metadata, sizeof(struct SequenceMetadata));
  stream.seekg(seq_metadata.filepath_pos);

  std::filesystem::path path;

  {
    std::vector<char> path_tmp;
    char c;
    while(true) {
      stream.get(c);
      path_tmp.push_back(c);
      if(c == 0)
        break;
    }
    path = std::filesystem::path(path_tmp.begin());
  }

  std::vector<SequenceMetadata> path_to_root;
  if(seq_index != metadata.root_index) {
    path_to_root.push_back(seq_metadata);
    while(path_to_root.back().parent != metadata.root_index) {
      stream.seekg(file_length - sizeof(struct FileMetadata) - (metadata.sequence_count - path_to_root.back().parent) * sizeof(struct SequenceMetadata));
      SequenceMetadata tmp;
      stream.read((char*) &tmp, sizeof(struct SequenceMetadata));
      path_to_root.emplace_back(tmp);
    }
  }

  stream.seekg(0);
  DynArray<char> ref(metadata.max_sequence_size);
  Profiler::profile("decompress_read_ref",
                    [&]() {std::copy_n(std::istreambuf_iterator<char>(stream), metadata.root_size, ref.begin());});

  DynArray<char> child(metadata.max_sequence_size);
  std::size_t seq_size = 0;
  for(auto it = path_to_root.rbegin(); it != path_to_root.rend(); ++it) {
    seq_size = 0;
    stream.seekg(it->factors_pos);
    for(std::size_t f = 0; f < it->factors; ++f) {
      filelength_type pos, len;
      stream.read((char*) &pos, sizeof(pos));
      stream.read((char*) &len, sizeof(len));
      if(len == 0) {
        child[seq_size++] = pos;
      }
      else {
        std::copy(ref.begin() + pos, ref.begin() + pos + len, child.begin() + seq_size);
        seq_size += len;
      }
    }
    std::swap(ref, child);
  }

  std::filesystem::create_directories(path.parent_path());
  std::ofstream output_file(path);
  if(seq_index == metadata.root_index) {
    seq_size = metadata.root_size;
  }
  std::copy_n(ref.begin(), seq_size, std::ostreambuf_iterator<char>(output_file));
  output_file.close();
}

/**
 * Write a description of the compressed HRLZ, that contains, in plain text:
 * n_sequences
 * root_id
 * data_source_directory
 * filename1
 * ...
 * filename_n
 * sequence_1 child1 child2 ...
 * ...
 * sequence_n child1 child2 ...
*/
void write_external_metadata(const DynArray<std::filesystem::path>& files, const std::unique_ptr<std::vector<std::size_t>[]>& arbor,
                             const std::filesystem::path& comp_dir, std::size_t root_id, const std::string& metadata_name) {
  std::filesystem::path output_metadata_path = comp_dir / metadata_name;
  std::ofstream ostream(output_metadata_path);
  int n = files.size();
  //Write number of files
  ostream << n << std::endl;
  //Write ID of root
  ostream << root_id << std::endl;
  //Write directory of origin of the files
  std::string origin_directory = std::filesystem::canonical(files[0].parent_path());
  ostream << origin_directory << std::endl;
  //Write filename of each compressed file
  for (int i = 0; i < n; i++) {
    ostream << files[i].filename().string() << std::endl;
  }
  print_arbor(ostream, n, arbor);
  ostream << "END" << std::endl;
  //That's it
  ostream.close();
}
/*
void write_chromosome_metadata(const DynArray<std::filesystem::path>& files, const std::unique_ptr<std::vector<size_t>[]>& arbor,
                               const std::filesystem::path& comp_dir, size_t root_id, size_t n_chromosomes) {
    std::filesystem::path output_metadata_path = comp_dir / "metadata_chr";
    //append
    size_t n = files.size();
    if(std::filesystem::exists(output_metadata_path)) {
        std::ofstream ostream(output_metadata_path, std::ios_base::app); //Note the append
        ostream << root_id << "\n";
        print_arbor(ostream, n, arbor);
    } else {
        std::ofstream ostream(output_metadata_path);
        ostream << n << std::endl;
        ostream << files[0].parent_path().parent_path() << std::endl; //Note the double parent path to go from chr file to the source
        for (size_t i = 0; i < n; i++)
            ostream << files[i].parent_path() << "\n";
        ostream << n_chromosomes << "\n";
        ostream << root_id << "\n";
        print_arbor(ostream, n, arbor);
    }
}
*/
void print_arbor(std::ostream& ostream, size_t n, const std::unique_ptr<std::vector<size_t>[]>& arbor) {
    //Write the metadata. Now we have to iterate through the adjacency list
    //If the list is empty, it will just be a newline
    //For each reference...
    for (size_t i = 0; i < n; i++) {
        //For each target...
        if (!arbor[i].empty()) {
            ostream << arbor[i][0];
            for (std::size_t j = 1; j < arbor[i].size(); j++) {
                ostream << " " << arbor[i][j];
            }
        }
        ostream << "\n";
    }
    ostream.flush();
}

/**
 * Read the description of the compressed HRLZ, and return a metadata object with the results.
*/
std::unique_ptr<ExternalMetadata> read_external_metadata(const std::filesystem::path& output_dir, const std::string& metadata_name) {
  std::unique_ptr<ExternalMetadata> metadata_obj = std::make_unique<ExternalMetadata>();
  std::filesystem::path output_metadata_file = output_dir / metadata_name;
  std::ifstream ifstream(output_metadata_file);
  if (!ifstream) {
    std::cerr << "Cannot open stream from " << output_metadata_file << std::endl; std::exit(1);
  }
  //Read number of sequences
  std::size_t n;
  ifstream >> n;
  if (!ifstream) {
    std::cerr << "Cannot read number of sequences from metadata" << std::endl; std::exit(1);
  }
  metadata_obj->n_sequences = n;
  //Read root ID
  std::size_t root_id;
  ifstream >> root_id;
  if (!ifstream) {
    std::cerr << "Cannot read root id from metadata" << std::endl; std::exit(1);
  }
  metadata_obj->root_id = root_id;
  //Read line break
  std::string waste; std::getline(ifstream, waste);
  //Read origin directory
  std::string origin_directory;
  std::getline(ifstream, origin_directory);
  metadata_obj->original_directory = origin_directory;
  //Read filenames
  std::unique_ptr<std::string[]> arr_filenames = std::make_unique<std::string[]>(n);
  for (std::size_t i = 0; i < n; i++) {
    std::string filename;
    std::getline(ifstream, filename);
    arr_filenames[i] = filename;
  }
  metadata_obj->filenames = std::move(arr_filenames);
  //Read adjacency list and store
  std::unique_ptr<std::vector<std::size_t>[]> adj_ptr = std::make_unique<std::vector<std::size_t>[]>(n);
  std::vector<std::size_t> *adj = adj_ptr.get();
  for (std::size_t i = 0; i < n; i++) {
    std::string line;
    std::getline(ifstream, line);
    std::vector<std::size_t> list = std::vector<std::size_t>();
    if (!line.empty()) {
      std::istringstream iss(line);
      std::size_t target;
      while (iss >> target) {
        list.push_back(target);
      }
    }
    adj[i] = list;
  }
  metadata_obj->arbor = std::move(adj_ptr);
  //We are done
  ifstream.close();
  return metadata_obj;
}

std::unique_ptr<FullTreeMetadata> read_metadata_full_tree(const std::filesystem::path& input_dir, const std::string& metadata_name) {
    std::unique_ptr<FullTreeMetadata> metadata_obj = std::make_unique<FullTreeMetadata>();
    std::filesystem::path input_metadata_file = input_dir / metadata_name;
    std::ifstream ifstream(input_metadata_file);
    if (!ifstream) {
        std::cerr << "Cannot open stream from " << input_metadata_file << std::endl; std::exit(1);
    }
    //Read number of sequences
    std::size_t n;
    ifstream >> n;
    if (!ifstream) {
        std::cerr << "Cannot read number of sequences from metadata" << std::endl; std::exit(1);
    }
    metadata_obj->n_sequences = n;
    //Read root ID
    std::size_t root_id;
    ifstream >> root_id;
    if (!ifstream) {
        std::cerr << "Cannot read root id from metadata" << std::endl; std::exit(1);
    }
    metadata_obj->root_id = root_id;
    //Read line break
    std::string waste; std::getline(ifstream, waste);
    //Read origin directory
    std::string origin_directory;
    std::getline(ifstream, origin_directory);
    metadata_obj->original_directory = origin_directory;
    //Read filenames
    std::unique_ptr<std::string[]> arr_filenames = std::make_unique<std::string[]>(n);
    for (std::size_t i = 0; i < n; i++) {
        std::string filename;
        std::getline(ifstream, filename);
        arr_filenames[i] = filename;
    }
    metadata_obj->filenames = std::move(arr_filenames);
    //Read adjacency list and store, while computing parents
    std::unique_ptr<std::vector<std::size_t>[]> adj_ptr = std::make_unique<std::vector<std::size_t>[]>(n);
    std::unique_ptr<size_t[]> parents = std::make_unique<size_t[]>(n);
    parents[root_id] = -1ULL;
    std::vector<std::size_t> *adj = adj_ptr.get();
    for (std::size_t i = 0; i < n; i++) {
        std::string line;
        std::getline(ifstream, line);
        std::vector<std::size_t> list = std::vector<std::size_t>();
        if (!line.empty()) {
            std::istringstream iss(line);
            std::size_t target;
            while (iss >> target) {
                list.push_back(target);
                parents[target] = i;
            }
        }
        adj[i] = list;
    }
    metadata_obj->arbor = std::move(adj_ptr);
    metadata_obj->parents = std::move(parents);
    //We are done
    ifstream.close();
    return metadata_obj;
}

/**
* LEGACY
 * DynArray<SequenceMetadata> compress_to_stream(std::ostream& stream, const DynArray<std::filesystem::path>& files, const std::vector<Edge>& edges, std::size_t root, const DynArray<std::filesystem::path>& sa) {
  std::size_t offset = 0;
  FileMetadata metadata;
  DynArray<SequenceMetadata> sequence_metadatas(files.size());

  auto root_file = read_compacted_DNA(files[root]); //Full file on a buffer
  stream.write(root_file.begin(), root_file.size());
  offset += root_file.size();
  sequence_metadatas[root].parent = std::numeric_limits<std::size_t>::max();
  sequence_metadatas[root].factors = root_file.size();
  sequence_metadatas[root].factors_pos = 0;
  sequence_metadatas[root].index_in_file = 0;

  metadata.sequence_count = files.size();
  metadata.root_index = root;
  metadata.root_size = root_file.size();
  metadata.max_sequence_size = metadata.root_size;

  std::size_t current_ref = root;
  RLZCompress compressor = RLZCompress::from(files[root], sa[root]);

  for(std::size_t i = 0; i < edges.size(); ++i) {
    auto& edge = edges[i];
    if(edge.from != current_ref) {
      compressor = RLZCompress::from(files[edge.from], sa[edge.from]);
      current_ref = edge.from;
    }
    FactorStreamWriter writer(&stream);
    auto sequence_size = compressor.compress(files[edge.to].c_str(), writer);
    metadata.max_sequence_size = std::max(metadata.max_sequence_size, sequence_size);
    sequence_metadatas[edge.to].factors = writer.factor_count();
    sequence_metadatas[edge.to].parent = edge.from;
    sequence_metadatas[edge.to].factors_pos = offset;
    sequence_metadatas[edge.to].index_in_file = i + 1;
    offset += writer.factor_count() * 2 * sizeof(filelength_type);
  }

  metadata.filepath_array_start = offset;

  for(std::size_t i = 0; i < files.size(); ++i) {
    const auto& file = files[i];
    std::size_t file_size = file.string().size();
    stream.write(file.c_str(), file_size);
    stream.put(0);
    sequence_metadatas[i].filepath_pos = offset;
    offset += file_size + 1;
  }

  stream.write((char*) sequence_metadatas.begin(), metadata.sequence_count * sizeof(struct SequenceMetadata));
  stream.write((char*) &metadata, sizeof(struct FileMetadata));
  return sequence_metadatas;
}
*/