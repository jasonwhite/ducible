/**
 * This MD5 implementation comes from: https://github.com/aappleby/smhasher
 *
 * Minor modifications were made by Jason White for better platform support.
 */
#pragma once

#include <stdlib.h> // For size_t

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief          MD5 context structure
 */
typedef struct
{
    unsigned int total[2];     /*!< number of bytes processed  */
    unsigned int state[4];     /*!< intermediate digest state  */
    unsigned char buffer[64];  /*!< data block being processed */
} md5_context;

/**
 * \brief          MD5 context setup
 *
 * \param ctx      context to be initialized
 */
void md5_starts(md5_context *ctx);

/**
 * \brief          MD5 process buffer
 *
 * \param ctx      MD5 context
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 */
void md5_update(md5_context *ctx, const unsigned char *input, size_t len);

/**
 * \brief          MD5 final digest
 *
 * \param ctx      MD5 context
 * \param output   MD5 checksum result
 */
void md5_finish(md5_context *ctx, unsigned char output[16]);

/**
 * \brief          Output = MD5( input buffer )
 *
 * \param input    buffer holding the  data
 * \param ilen     length of the input data
 * \param output   MD5 checksum result
 */
void md5(const unsigned char *input, size_t len, unsigned char output[16]);

#ifdef __cplusplus
}
#endif
