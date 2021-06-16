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

#ifndef INCLUDE_SCHIFRA_REED_SOLOMON_FILE_ENCODER_HPP
#define INCLUDE_SCHIFRA_REED_SOLOMON_FILE_ENCODER_HPP

#include <cstring>

#include <iostream>

#include <fstream>

#include "schifra_reed_solomon_block.hpp"

#include "schifra_reed_solomon_encoder.hpp"

#include "schifra_fileio.hpp"


namespace schifra {

    namespace reed_solomon {

        template < std::size_t part_size_bytes, std::size_t code_length, std::size_t fec_length, std::size_t data_length = code_length - fec_length >
            class file_encoder {
                public:

                    typedef encoder < code_length, fec_length > encoder_type;
                typedef typename encoder_type::block_type block_type;

                file_encoder(const encoder_type & encoder,
                    const std::string & input_file_name,
                        const std::string & output_file_name) {

                    const std::size_t columns = 219;
                    const std::size_t rows = part_size_bytes / columns;
                    const std::size_t one_chunk_size_bytes = rows * data_length;

                    char * fec_buffer_ = new char[fec_length];
                    char ** chunk_data = new char * [rows];
                   
                    bool remaining_bytes_exists = false;

                    std::size_t file_size = schifra::fileio::file_size(input_file_name);
                    if (file_size == 0) {
                        std::cout << "reed_solomon::file_encoder() - Error: input file has ZERO size." << std::endl;
                        return;
                    }

                    std::ifstream in_stream(input_file_name.c_str(), std::ios::binary);
                    if (!in_stream) {
                        std::cout << "reed_solomon::file_encoder() - Error: input file could not be opened." << std::endl;
                        return;
                    }
                    std::cout << "Size of the file is" << " " << file_size << " " << "bytes" << std::endl;

                    std::ofstream out_stream(output_file_name.c_str(), std::ios::binary);
                    if (!out_stream) {
                        std::cout << "reed_solomon::file_encoder() - Error: output file could not be created." << std::endl;
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
                        int a = 0;
                        
                        while (remaining_bytes >= 219) {
                            a += 5;
                            chunk_data[index] = new char[code_length];
                            in_stream.read( & chunk_data[index][0], static_cast < std::streamsize > (219));
                           
                            // Adding 4 bytes end of block
                            char *p = (char*)&a;
                            for (int i = 219; i < 223 ; ++i)
                                   chunk_data[index][i] =  static_cast<int>(*p++);

                            process_block(encoder, out_stream, chunk_data[index], fec_buffer_, data_length);
                            remaining_bytes -= 219;
                            index++;
                        }

                        if (remaining_bytes > 0) {
                            remaining_bytes_exists = true;
                            chunk_data[index] = new char[remaining_bytes + fec_length];
                            in_stream.read( & chunk_data[index][0], static_cast < std::streamsize > (remaining_bytes));
                            process_block(encoder, out_stream, chunk_data[index], fec_buffer_, remaining_bytes);
                            index++;
                        }
                        if (remaining_bytes_exists) {
                            // If we have remaining_bytes, handle it here
                            // Transpose semi block
                            for (std::size_t i = 0; i < code_length; ++i) {
                                for (std::size_t j = 0; j < index - 1; ++j) {
                                    out_stream << chunk_data[j][i];
                                }
                            }
                            for (std::size_t i = 0; i < remaining_bytes + fec_length; ++i) {

                                out_stream << chunk_data[index - 1][i];
                            }
                        } else {
                            // Transpose full block
                            for (std::size_t i = 0; i < code_length; ++i) {
                                for (std::size_t j = 0; j < index; ++j) {
                                    out_stream << chunk_data[j][i];
                                }
                            }

                        }
                    }

                    in_stream.close();
                    out_stream.close();
                }

                private:

                    inline void process_block(const encoder_type & encoder,
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
                            std::cout << "reed_solomon::file_encoder.process_block() - Error during encoding of block!" << std::endl;
                            return;
                        }

                        for (std::size_t i = 0; i < fec_length; ++i) {
                            fec_buffer_[i] = static_cast < char > (block_.fec(i) & 0xFF);
                        }

                        for (std::size_t i = 0, j = read_amount; i < fec_length; ++i, ++j) {
                            data_buffer_[j] = fec_buffer_[i];
                        }
                    }

                block_type block_;
              
            };

    } // namespace reed_solomon

} // namespace schifra

#endif