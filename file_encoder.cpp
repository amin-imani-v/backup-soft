#include <iostream>

#include <streambuf>

#include <fstream>

#include <unistd.h>

#include <vector>

#include <iterator>

#include <cstddef>

#include <string>

#include "schifra/schifra_galois_field.hpp"

#include "schifra/schifra_galois_field_polynomial.hpp"

#include "schifra/schifra_sequential_root_generator_polynomial_creator.hpp"

#include "schifra/schifra_reed_solomon_encoder.hpp"

#include "schifra/schifra_reed_solomon_decoder.hpp"

#include "schifra/schifra_reed_solomon_block.hpp"

#include "schifra/schifra_error_processes.hpp"

#include "schifra/schifra_fileio.hpp"

const std::size_t code_length = 255;
const std::size_t fec_length = 32;
const std::size_t data_length = code_length - fec_length;

typedef schifra::reed_solomon::encoder < code_length, fec_length, data_length > encoder_t;
typedef typename encoder_t::block_type block_type;
block_type block_;

void encode_block(const encoder_t & encoder,
    std::ofstream & out_stream,
    char * data_buffer_,
    char * fec_buffer_,
    const std::size_t & read_amount) {

    for (std::size_t i = 0; i < read_amount; ++i) {
        block_.data[i] = (data_buffer_[i] & 0xFF);
    }

    if (read_amount < data_length) {
        for (std::size_t i = read_amount; i < data_length; ++i) {
            block_.data[i] = 0x00;
        }
    }

    if (!encoder.encode(block_)) {
        std::cout << "Error during encoding of block!" << std::endl;
        return;
    }

    for (std::size_t i = 0; i < fec_length; ++i) {
        fec_buffer_[i] = static_cast < char > (block_.fec(i) & 0xFF);
    }

    out_stream.write( & data_buffer_[0], static_cast < std::streamsize > (read_amount));
    out_stream.write( & fec_buffer_[0], fec_length);
}

int main() {
    
    const std::string input_filename = "input.txt";
    const std::string output_filename = "encoded_file.txt";

    const int one_chunk_size_bytes = 1048546;
    char * data_buffer_ = new char[data_length];
    char * fec_buffer_ = new char[fec_length];
    char * chunk_data;

    const std::size_t field_descriptor = 8;
    const std::size_t generator_polynomial_index = 120;
    const std::size_t generator_polynomial_root_count = 32;

    const schifra::galois::field field(field_descriptor,
        schifra::galois::primitive_polynomial_size06,
        schifra::galois::primitive_polynomial06);
    schifra::galois::field_polynomial generator_polynomial(field);

    if (
        !schifra::make_sequential_root_generator_polynomial(field,
            generator_polynomial_index,
            generator_polynomial_root_count,
            generator_polynomial)
    ) {
        std::cout << "Error - Failed to create sequential root generator!" << std::endl;
        return 1;
    }

    const encoder_t encoder(field, generator_polynomial);

    std::size_t file_size = schifra::fileio::file_size(input_filename);
    if (file_size == 0) {
        std::cout << "Error: input file has ZERO size." << std::endl;
        return 1;
    }
    std::cout << "Size of the file is" << " " << file_size << " " << "bytes" << std::endl;

    std::ifstream in_stream(input_filename.c_str(), std::ios::binary);
    if (!in_stream) {
        std::cout << "Error: input file could not be opened." << std::endl;
        return 1;
    }

    std::ofstream out_stream(output_filename.c_str(), std::ios::binary);
    if (!out_stream) {
        std::cout << "Error: output file could not be created." << std::endl;
        return 1;
    }
    
    for (int chunk = 0; chunk <= file_size / one_chunk_size_bytes; chunk++) {
        int length = 0;
        std::size_t remaining_bytes = 0;
       
        if (file_size - (chunk * one_chunk_size_bytes) >= one_chunk_size_bytes) {
            length = one_chunk_size_bytes;
            remaining_bytes = length;
        } else {
            length = file_size - (chunk * one_chunk_size_bytes);
            remaining_bytes = length;
        }

        chunk_data = new char[length];
        in_stream.read(chunk_data, length);
        int index = 0;
        while (remaining_bytes >= data_length) {
            for (int i = 0; i < data_length; i++, index++) {
                data_buffer_[i] = chunk_data[index];
            }
            encode_block(encoder, out_stream, data_buffer_, fec_buffer_, data_length);
            remaining_bytes -= data_length;
        }
        if (remaining_bytes > 0) {
            for (int i = 0 ; i < remaining_bytes; i++, index++) {
                data_buffer_[i] = chunk_data[index];
            }
            encode_block(encoder, out_stream, data_buffer_, fec_buffer_, remaining_bytes);
        }
        printf("chunk %d : %d bytes done!\n", chunk, length);
    }
    in_stream.close();
    out_stream.close();
    return 0;
}