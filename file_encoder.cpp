#include <cstddef>
#include <string>

#include "schifra/schifra_galois_field.hpp"
#include "schifra/schifra_sequential_root_generator_polynomial_creator.hpp"
#include "schifra/schifra_reed_solomon_encoder.hpp"
#include "schifra/schifra_reed_solomon_file_encoder.hpp"

int main()
{
   const std::size_t field_descriptor    =   8;
   const std::size_t gen_poly_index      = 120;
   const std::size_t gen_poly_root_count =   32;
   const std::size_t code_length         = 255;
   const std::size_t fec_length          =   32;
   const std::size_t part_size_bytes     = 1048576;
   const std::string input_file_name     = "input3.txt";
   const std::string output_file_name    = "encoded_file.txt";

   typedef schifra::reed_solomon::encoder<code_length,fec_length> encoder_t;
   typedef schifra::reed_solomon::file_encoder<part_size_bytes,code_length,fec_length> file_encoder_t;

   const schifra::galois::field field(field_descriptor,
                                      schifra::galois::primitive_polynomial_size06,
                                      schifra::galois::primitive_polynomial06);

   schifra::galois::field_polynomial generator_polynomial(field);

   if (
        !schifra::make_sequential_root_generator_polynomial(field,
                                                            gen_poly_index,
                                                            gen_poly_root_count,
                                                            generator_polynomial)
      )
   {
      std::cout << "Error - Failed to create sequential root generator!" << std::endl;
      return 1;
   }

   const encoder_t rs_encoder(field,generator_polynomial);

   file_encoder_t(rs_encoder, input_file_name, output_file_name);

   return 0;
}
