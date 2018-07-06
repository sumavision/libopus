#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "opus.h"
#include "opus_defines.h"


#define TEST_OPUS_SAMPLE_RATE 8000
#define TEST_OPUS_CHANNEL_NUM 1

/** 10ms data size(2.5|5|10|20|40|60|80|100|120)*/
#define TEST_OPUS_FRAME_TIME_MS       20
#define TEST_OPUS_MAX_SAMPLING_RATE   48000
/** 10ms*/
#define TEST_OPUS_MAX_FRAME_SIZE      TEST_OPUS_MAX_SAMPLING_RATE/1000*TEST_OPUS_FRAME_TIME_MS

#define TEST_OPUS_MAX_PAYLOAD_BYTES   255*4+255

static opus_uint32 char_to_int(unsigned char ch[4])
{
    return ((opus_uint32)ch[0]<<24) | ((opus_uint32)ch[1]<<16)
         | ((opus_uint32)ch[2]<< 8) |  (opus_uint32)ch[3];
}

static int test_opus_decode(uint8_t* flie_in,uint8_t* file_opus,opus_int32 fs,int channels)
{
    int ret = 0;
    OpusDecoder *opusDec=NULL;

    opusDec = opus_decoder_create(fs, channels,&ret);
    if (ret != OPUS_OK)
    {
       fprintf(stderr, "Cannot create decoder: %s\n", opus_strerror(ret));
       return -1;
    }

    FILE *fin = fopen(flie_in, "rb");
    if (!fin)
    {
        fprintf (stderr, "Could not open input file %s\n", flie_in);
        ret = -1;
        goto opus_dec_dstroy;
    }
    FILE *fout = fopen(file_opus, "wb+");
    if (!fout)
    {
        fprintf (stderr, "Could not open output file %s\n", file_opus);
        ret = -1;
        goto opus_fin_close;
    }

    opus_int16 frame_size = fs*TEST_OPUS_FRAME_TIME_MS/1000;

    opus_int16 *out = (opus_int16*)malloc(TEST_OPUS_MAX_FRAME_SIZE*channels*sizeof(opus_int16));
    uint8_t *fbytes = (uint8_t*)malloc(TEST_OPUS_MAX_FRAME_SIZE*channels*sizeof(opus_int16));
    uint8_t *opus_data = (uint8_t*)calloc(TEST_OPUS_MAX_PAYLOAD_BYTES,sizeof(uint8_t));

    int len = 0,num_read = 0;
    opus_uint32 enc_final_range = 0;
    opus_decoder_init();
    while (1)
    {
        uint8_t ch[4];
        num_read = fread(ch, 1, sizeof(ch), fin);
        if (num_read!=4)
        {
            fprintf(stderr, "current frame read enc length fail: readSize(%d)\n",num_read);
            break;
        }
        len = char_to_int(ch);
        if (len >TEST_OPUS_MAX_PAYLOAD_BYTES || len<0)
        {
            fprintf(stderr, "Invalid payload length: %d\n",len);
            break;
        }
        num_read = fread(ch, 1, 4, fin);
        if (num_read!=4)
        {
            fprintf(stderr, "current frame read enc_final_range fail: readSize(%d)\n",num_read);
            break;
        }
        enc_final_range = char_to_int(ch);

        num_read = fread(opus_data, 1, len, fin);
        if (num_read!=(size_t)len)
        {
            fprintf(stderr, "Ran out of input, "
                            "expecting %d bytes got %d\n",
                            len,(int)num_read);
            break;
        }

        int output_samples = opus_decode(opusDec, opus_data, (opus_int32)len, out,frame_size,0);
        if(output_samples < 0)
        {
            fprintf(stderr, "error decoding frame: %s\n",opus_strerror(output_samples));
            ret = -1;
            break;
        }
        for(int i=0;i<output_samples*channels;i++)
        {
           opus_int16 s;
           s=out[i];
           fbytes[2*i]=s&0xFF;
           fbytes[2*i+1]=(s>>8)&0xFF;
        }
        if (fwrite(fbytes, sizeof(opus_int16)*channels, output_samples, fout) != (unsigned)(output_samples))
        {
           fprintf(stderr, "Error writing.\n");
           ret = -1;
           break;
        }
    }

opus_buff_free:
    free(out);
    free(fbytes);
    free(opus_data);
opus_file_close:
    fclose(fout);
opus_fin_close:
    fclose(fin);
opus_dec_dstroy:
    opus_decoder_destroy(opusDec);

    return ret;
}
int main()
{
    test_opus_decode("opus_8k_16bit.opus","opus_dec_8k_16bit.pcm",TEST_OPUS_SAMPLE_RATE,TEST_OPUS_CHANNEL_NUM);
}
