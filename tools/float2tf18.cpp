#include <cstdint>
#include <iostream>
#include <string>

float tf18_to_float(uint32_t tf18_bits)
{
    uint32_t fp32_sign = (tf18_bits & 0x20000) >> 17;

    uint32_t fp32_exponent = (tf18_bits & 0x1fc00) >> 10;
    fp32_exponent += 64;    // Correct for bias

    uint32_t fp32_mantissa = (tf18_bits & 0x003ff);

    uint32_t fp32_bits = (fp32_sign << 31)
                       | (fp32_exponent << 23)
                       | (fp32_mantissa << 13);

    float *fp32_ptr = reinterpret_cast<float*>(&fp32_bits);
    return *fp32_ptr;
}

uint32_t float_to_tf18(double f)
{
    uint64_t* bits = reinterpret_cast<uint64_t*>(&f);
    uint64_t rawBits = *bits;
    uint64_t input_sign = (rawBits >> 63) & 0x1;
    uint32_t input_exp = (rawBits >> 52) & 0x7FF;
    uint64_t input_mantissa = rawBits & 0x000FFFFFFFFFFFFF;
    uint32_t output_sign = input_sign;
    uint32_t output_exp;
    uint64_t output_mantissa;
    uint32_t output_rawBits;
    output_exp = input_exp; // converts bias 127 to bias 63
    if (output_exp > 1023) {
        output_exp -= 960;
        if (output_exp > 127)
            output_exp = 127;
        output_exp <<= 10;
    }
    // else if (output_exp > 127){
    //     output_exp = (output_exp - 64);
    //     output_exp <<= 9;
    // }
    else if (output_exp > 960){
        output_exp = (output_exp - 960) & 0x7F;
        output_exp <<= 10;
    }

    if (input_mantissa & 0x20000000000){
        output_mantissa = (input_mantissa >> 42) + 1;
    }
    else{
        output_mantissa = input_mantissa >> 42;
    }
    
    output_rawBits = (output_sign << 17) | output_exp | output_mantissa;
    return output_rawBits;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <float>" << std::endl;
        return 1;
    }

    std::string input = argv[1];

    if (input.size() > 1 && input[0] == '0' && input[1] == 'x') {
        if (input.size() == 8) {
            std::cout << tf18_to_float(std::stoul(input, nullptr, 16) >> 6) << "\n";
        } else {
            std::cout << tf18_to_float(std::stoul(input, nullptr, 16)) << "\n";
        }
    } else {
        std::cout << "0x" << std::hex
                  << float_to_tf18(std::stod(input)) << "\n";
    }
}
