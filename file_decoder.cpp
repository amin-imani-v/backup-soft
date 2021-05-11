#include <cstddef>
#include <string>

#include "schifra/schifra_galois_field.hpp"
#include "schifra/schifra_sequential_root_generator_polynomial_creator.hpp"
#include "schifra/schifra_reed_solomon_decoder.hpp"
#include "schifra/schifra_reed_solomon_file_decoder.hpp"


int main()
{
   const std::size_t field_descriptor    =   8;
   const std::size_t gen_poly_index      = 120;
   const std::size_t code_length         = 255;
   const std::size_t fec_length          =   32;
   const std::string input_file_name     = "encoded_file.txt";
   const std::string output_file_name    = "decoded_file.txt";

   typedef schifra::reed_solomon::decoder<code_length,fec_length> decoder_t;
   typedef schifra::reed_solomon::file_decoder<code_length,fec_length> file_decoder_t;

   const schifra::galois::field field(field_descriptor,
                                      schifra::galois::primitive_polynomial_size06,
                                      schifra::galois::primitive_polynomial06);

   const decoder_t rs_decoder(field,gen_poly_index);

   file_decoder_t(rs_decoder, input_file_name, output_file_name);

   return 0;
}
