//File generated from template by ad-C.

//vector.adc

typedef struct v2f_s
{
    union
    {
        struct
        {
            float x;
            float y;
        };
        float d[2];
    };
} v2f;

typedef struct v3f_s
{
    union
    {
        struct
        {
            float x;
            float y;
            float z;
        };
        float d[3];
    };
} v3f;

//vector.adc

typedef struct v2i_s
{
    union
    {
        struct
        {
            int32_t x;
            int32_t y;
        };
        int32_t d[2];
    };
} v2i;

typedef struct v3i_s
{
    union
    {
        struct
        {
            int32_t x;
            int32_t y;
            int32_t z;
        };
        int32_t d[3];
    };
} v3i;

