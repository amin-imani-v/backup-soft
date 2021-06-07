/*
(**************************************************************************)
(*                                                                        *)
(*                                Schifra                                 *)
(*                Reed-Solomon Error Correcting Code Library              *)
(*                                                                        *)
(* Release Version 0.0.1                                                  *)
(* http://www.schifra.com                                                 *)
(* Copyright (c) 2000-2020 Arash Partow, All Rights Reserved.             *)
(*                                                                        *)
(* The Schifra Reed-Solomon error correcting code library and all its     *)
(* components are supplied under the terms of the General Schifra License *)
(* agreement. The contents of the Schifra Reed-Solomon error correcting   *)
(* code library and all its components may not be copied or disclosed     *)
(* except in accordance with the terms of that agreement.                 *)
(*                                                                        *)
(* URL: http://www.schifra.com/license.html                               *)
(*                                                                        *)
(**************************************************************************)
*/

#ifndef INCLUDE_SCHIFRA_REED_SOLOMON_FILE_DECODER_HPP
#define INCLUDE_SCHIFRA_REED_SOLOMON_FILE_DECODER_HPP

#include <iostream>

#include <fstream>

#include "schifra_reed_solomon_block.hpp"

#include "schifra_reed_solomon_decoder.hpp"

#include "schifra_fileio.hpp"


namespace schifra {

    namespace reed_solomon {

        template <std::size_t part_size_bytes, std::size_t code_length, std::size_t fec_length, std::size_t data_length = code_length - fec_length >
            class file_decoder {
                public:

                    typedef decoder < code_length, fec_length > decoder_type;
                	  typedef typename decoder_type::block_type block_type;

                file_decoder(const decoder_type & decoder,
                    	 	const std::string & input_file_name,
                            const std::string & output_file_name,
                            bool debug,
                            int pause_block) : current_block_index_(0) {

                    const std::size_t columns = data_length;
                    const std::size_t rows = part_size_bytes / columns;
                    const std::size_t one_chunk_size_bytes = rows * code_length;

                    char * fec_buffer_ = new char[fec_length];
                    char ** chunk_data = new char * [rows];
                    bool remaining_bytes_exists = false;
                    for (std::size_t i = 0; i < rows; ++i)
                        chunk_data[i] = new char[code_length];

                    std::size_t file_size = schifra::fileio::file_size(input_file_name);
                    if (file_size == 0) {
                        std::cout << "reed_solomon::file_decoder() - Error: input file has ZERO size." << std::endl;
                        return;
                    }
                    std::cout << "Size of the file is" << " " << file_size << " " << "bytes" << std::endl;
                    std::ifstream in_stream(input_file_name.c_str(), std::ios::binary);
                    if (!in_stream) {
                        std::cout << "reed_solomon::file_decoder() - Error: input file could not be opened." << std::endl;
                        return;
                    }

                    std::ofstream out_stream(output_file_name.c_str(), std::ios::binary);
                    if (!out_stream) {
                        std::cout << "reed_solomon::file_decoder() - Error: output file could not be created." << std::endl;
                        return;
                    }

                    for (std::size_t chunk = 0; chunk <= file_size / one_chunk_size_bytes; chunk++) {
                        std::size_t length = 0;
                        std::size_t remaining_bytes = 0;

                        if (file_size - (chunk * one_chunk_size_bytes) >= one_chunk_size_bytes) {
                            length = one_chunk_size_bytes;
                            remaining_bytes = length;
                        } else {
                            length = file_size - (chunk * one_chunk_size_bytes);
                            remaining_bytes = length;
                        }

                        std::size_t index = 0;
                        for (std::size_t i = 0; i < code_length; ++i) {
                            remaining_bytes = length;
                            for (std::size_t j = 0; remaining_bytes >= code_length; ++j) {
                                in_stream.read( & chunk_data[j][i], 1);
                                index = j + 1;
                                remaining_bytes -= code_length;
                            }

                        }

                        if (remaining_bytes > 0) {
                            remaining_bytes_exists = true;
                            for (std::size_t i = 0; i < remaining_bytes; ++i) {
                                in_stream.read( & chunk_data[index][i], 1);
                            }
                            index++;
                        }

                        for (std::size_t i = 0; i < index; ++i) {
                            if (remaining_bytes_exists && i == index - 1) {
                                process_partial_block(decoder, chunk_data[i], out_stream, remaining_bytes);
                                out_stream.write( & chunk_data[i][0], static_cast < std::streamsize > (remaining_bytes - fec_length));
                                if(debug){
                                    std::cout<< "Decoding block " << current_block_index_<< std::endl;
                                    for(std::size_t j = 0; j < code_length; ++j){
                                        std::cout<< chunk_data[i][j];
                                    }
                                    std::cout<<std::endl << std::endl;
                                }
                            } else {
                                process_complete_block(decoder, chunk_data[i], out_stream);
                                out_stream.write( & chunk_data[i][0], static_cast < std::streamsize > (data_length));
                                if(debug){
                                    std::cout<< "Decoding block " << current_block_index_<< std::endl;
                                    for(std::size_t j = 0; j < code_length; ++j){
                                        std::cout<< chunk_data[i][j];
                                    }
                                    std::cout<<std::endl << std::endl;
                                    if(pause_block != -1 && current_block_index_ >= pause_block){
                                        getchar();
                                    }
                                }
                            }
                            current_block_index_++;
                        }
                    }
                    in_stream.close();
                    out_stream.close();
                }

                private:

                    inline void process_complete_block(const decoder_type & decoder,
                        char * buffer_,
                        std::ofstream & out_stream) {
                        copy < char, code_length, fec_length > (buffer_, code_length, block_);

                        if (!decoder.decode(block_)) {
                            std::cout << "reed_solomon::file_decoder.process_complete_block() - Error during decoding  of block " << current_block_index_ << std::endl;
                            return;
                        }

                        for (std::size_t i = 0; i < data_length; ++i) {
                            buffer_[i] = static_cast < char > (block_[i]);
                        }

                    }

                inline void process_partial_block(const decoder_type & decoder,
                    char * buffer_,
                    std::ofstream & out_stream,
                    const std::size_t & read_amount) {
                    if (read_amount <= fec_length) {
                        std::cout << "reed_solomon::file_decoder.process_partial_block() - Error during decoding of block " << current_block_index_ << std::endl;
                        return;
                    }

                    for (std::size_t i = 0; i < (read_amount - fec_length); ++i) {
                        block_.data[i] = static_cast < typename block_type::symbol_type > (buffer_[i]);
                    }

                    if ((read_amount - fec_length) < data_length) {
                        for (std::size_t i = (read_amount - fec_length); i < data_length; ++i) {
                            block_.data[i] = 0;
                        }
                    }

                    for (std::size_t i = 0; i < fec_length; ++i) {
                        block_.fec(i) = static_cast < typename block_type::symbol_type > (buffer_[(read_amount - fec_length) + i]);
                    }

                    if (!decoder.decode(block_)) {
                        std::cout << "reed_solomon::file_decoder.process_partial_block() - Error during decoding of block " << current_block_index_ << std::endl;
                        return;
                    }

                    for (std::size_t i = 0; i < (read_amount - fec_length); ++i) {
                        buffer_[i] = static_cast < char > (block_.data[i]);
                    }
                }

                block_type block_;
                std::size_t current_block_index_;
            };

    } // namespace reed_solomon

} // namespace schifra

#endif