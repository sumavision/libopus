#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "opus.h"
#include "opus_defines.h"


#define TEST_OPUS_SAMPLE_RATE 8000
#define TEST_OPUS_CHANNEL_NUM 1
#define TEST_OPUS_ENC_BIT_RATE 64000

/** 10ms data size(2.5|5|10|20|40|60|80|100|120)*/
#define TEST_OPUS_FRAME_TIME_MS       20
#define TEST_OPUS_MAX_SAMPLING_RATE   48000
/** 10ms*/
#define TEST_OPUS_MAX_FRAME_SIZE      TEST_OPUS_MAX_SAMPLING_RATE/1000*TEST_OPUS_FRAME_TIME_MS

#define TEST_OPUS_MAX_PAYLOAD_BYTES   255*4+255

static void int_to_char(opus_uint32 i, unsigned char ch[4])
{
    ch[0] = i>>24;
    ch[1] = (i>>16)&0xFF;
    ch[2] = (i>>8)&0xFF;
    ch[3] = i&0xFF;
}

static int test_opus_encode(uint8_t* flie_in,uint8_t* file_opus,opus_int32 fs,int channels)
{
    int ret = 0;
    OpusEncoder *opusEnc=NULL;

    opusEnc = opus_encoder_create(fs, channels, OPUS_APPLICATION_VOIP, &ret);
    if (ret != OPUS_OK)
    {
       fprintf(stderr, "Cannot create encoder: %s\n", opus_strerror(ret));
       return -1;
    }
    opus_encoder_ctl(opusEnc, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(opusEnc, OPUS_SET_BITRATE(OPUS_AUTO));//使用AUTO主要是应为视频会议过程中,大部分场景是不说话的,减少带宽占用
    opus_encoder_ctl(opusEnc, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND));
    opus_encoder_ctl(opusEnc, OPUS_SET_VBR(1));//0:CBR, 1:VBR
    opus_encoder_ctl(opusEnc, OPUS_SET_VBR_CONSTRAINT(0));//0:Unconstrained VBR., 1:Constrained VBR.
    opus_encoder_ctl(opusEnc, OPUS_SET_COMPLEXITY(8));//range:0~10
    opus_encoder_ctl(opusEnc, OPUS_SET_FORCE_CHANNELS(OPUS_AUTO)); //1:Forced mono, 2:Forced stereo
    opus_encoder_ctl(opusEnc, OPUS_SET_APPLICATION(OPUS_APPLICATION_VOIP));//
    opus_encoder_ctl(opusEnc, OPUS_SET_INBAND_FEC(0));//0:Disable, 1:Enable

    FILE *fin = fopen(flie_in, "rb");
    if (!fin)
    {
        fprintf (stderr, "Could not open input file %s\n", flie_in);
        ret = -1;
        goto opus_enc_dstroy;
    }
    FILE *fout = fopen(file_opus, "wb+");
    if (!fout)
    {
        fprintf (stderr, "Could not open output file %s\n", file_opus);
        ret = -1;
        goto opus_fin_close;
    }

    opus_int16 frame_size = fs*TEST_OPUS_FRAME_TIME_MS/1000;

    opus_int16 *in = (opus_int16*)malloc(TEST_OPUS_MAX_FRAME_SIZE*channels*sizeof(opus_int16));
    /* We need to allocate for 16-bit PCM data, but we store it as unsigned char. */
    uint8_t *fbytes = (uint8_t*)malloc(TEST_OPUS_MAX_FRAME_SIZE*channels*sizeof(opus_int16));
    uint8_t *opus_data = (uint8_t*)calloc(TEST_OPUS_MAX_PAYLOAD_BYTES,sizeof(uint8_t));

    int num_read = 0,i = 0;//,remaining = 0,nb_encoded = 0;
    opus_uint32 enc_final_range = 0;
    while (1)
    {
        num_read = (int)fread(fbytes, sizeof(short)*channels, frame_size, fin);
        if (num_read!=(size_t)frame_size)
        {
            fprintf(stderr, "Ran out of input, "
                            "expecting %d bytes got %d\n",
                            frame_size,(int)num_read);
            break;
        }
        for(i=0;i<num_read*channels;i++)
        {
            opus_int32 s;
            s=fbytes[2*i+1]<<8|fbytes[2*i];
            s=((s&0xFFFF)^0x8000)-0x8000;
            //in[i+remaining*channels]=s;
            in[i]=s;
        }
        //if (num_read+remaining < frame_size)
        //{
        //    for (i=(num_read+remaining)*channels;i<frame_size*channels;i++)
        //       in[i] = 0;
        //    break;
       // }
        int len = opus_encode(opusEnc, in, frame_size, opus_data,TEST_OPUS_MAX_PAYLOAD_BYTES);
        if(len < 0)
        {
            fprintf(stderr, "error encoding frame: %s\n",opus_strerror(len));
            ret = -1;
            break;
        }
       // nb_encoded = opus_packet_get_samples_per_frame(opus_data, fs)*opus_packet_get_nb_frames(opus_data, len);
       // remaining = frame_size-nb_encoded;
       // for(i=0;i<remaining*channels;i++)
       //    in[i] = in[nb_encoded*channels+i];

        //写入编码长度
        uint8_t int_field[4];
        int_to_char(len, int_field);
        if (fwrite(int_field, 1, sizeof(int_field), fout) != sizeof(int_field)) {
           fprintf(stderr, "Error writing.\n");
           ret = -1;
           break;
        }
        //写入enc_final_range---测试
        opus_encoder_ctl(opusEnc, OPUS_GET_FINAL_RANGE(&enc_final_range));
        int_to_char(enc_final_range, int_field);
        if (fwrite(int_field, 1, sizeof(int_field), fout) != sizeof(int_field)) {
           fprintf(stderr, "Error writing.\n");
           ret = -1;
           break;
        }
        //写入编码数据
        if (fwrite(opus_data, sizeof(uint8_t), len, fout) != (size_t)len)
        {
            fprintf(stderr, "Error writing.\n");
            ret = -1;
            break;
        }
    }

opus_buff_free:
    free(in);
    free(fbytes);
    free(opus_data);
opus_file_close:
    fclose(fout);
opus_fin_close:
    fclose(fin);
opus_enc_dstroy:
    opus_encoder_destroy(opusEnc);

    return ret;
}
int main()
{
    test_opus_encode("audio_8k_16bit.pcm","opus_8k_16bit.opus",TEST_OPUS_SAMPLE_RATE,TEST_OPUS_CHANNEL_NUM);
}
