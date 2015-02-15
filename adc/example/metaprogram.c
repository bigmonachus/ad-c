#include <adc.h>

int main()
{
    adc_clear_file("vector.h");
    adc_expand(
            "vector.h",
            "vector.adc",
            3,  // Number of bindings
            // Bindings:
            "name_2", "v2f",
            "name_3", "v3f",
            "type", "float");

    adc_expand(
            "vector.h",
            "vector.adc",
            3,  // Number of bindings
            // Bindings:
            "name_2", "v2i",
            "name_3", "v3i",
            "type", "int32_t");

    adc_type_info(
            "./dummy_project/types.h",
            "./dummy_project");
}
