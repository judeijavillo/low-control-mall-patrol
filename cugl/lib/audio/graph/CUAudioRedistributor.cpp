//
//  CUAudioRedistributor.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a graph node for converting from one set of channels
//  to a different set of channels (e.g. mono to stereo or 5.1 down to mono).
//  It is necessary because some devices (notably MSI laptops) will always
//  give you audio devices with 5.1 or 7.1 channels, even when you ask for
//  stereo.
//
//  Historically, this conversion was done with SDL_AudioStream. And unlike
//  resampling, this is a conversion that works properly in SDL_AudioStream.
//  However, because we had to drop SDL_AudioStream for resampling, we decided
//  to drop it entirely.
//
//  CUGL MIT License:
//
//     This software is provided 'as-is', without any express or implied
//     warranty.  In no event will the authors be held liable for any damages
//     arising from the use of this software.
//
//     Permission is granted to anyone to use this software for any purpose,
//     including commercial applications, and to alter it and redistribute it
//     freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not
//     be misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//
//  Author: Walker White
//  Version: 6/5/21
//
#include <cugl/audio/graph/CUAudioRedistributor.h>
#include <cugl/audio/CUAudioDevices.h>
#include <cugl/util/CUDebug.h>
#include <cmath>

using namespace cugl::audio;

// The following are all safe to do in place
#pragma mark Downward Conversions
/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in stereo, while output should
 * support a mono stream with the same number of audio frames.
 * The value size is specified in terms of frames, not samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_stereo_to_mono(const float* input, float* output, size_t size) {
    const float *src = input;
    float *dst = output;
    for (size_t ii = 0; ii < size; ii++, src += 2) {
        *(dst++) = (src[0] + src[1]) * 0.5f;
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in tri-valued stereo (left-right-center),
 * while output should support a stereo stream with the same number
 * of audio frames. The value size is specified in terms of frames,
 * not samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_tri_to_stereo(const float* input, float* output, size_t size) {
    const float *src = input;
    float *dst = output;

    /* SDL's 5.1 layout: FL+FR+FC+LFE+BL+BR */
    for (size_t ii = 0; ii < size; ii++, src += 3, dst += 2) {
        const float front_center_distributed = src[2] * 0.55f;
        dst[0] = (src[0] + front_center_distributed)/1.5f;  /* left */
        dst[1] = (src[1] + front_center_distributed)/1.5f;  /* right */
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in quadraphonic format, while output
 * should support a stereo stream with the same number of audio
 * frames. The value size is specified in terms of frames, not
 * samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_quad_to_stereo(const float* input, float* output, size_t size) {
    const float *src = input;
    float *dst = output;

    /* SDL's 5.1 layout: FL+FR+FC+LFE+BL+BR */
    for (size_t ii = 0; ii < size; ii++, src += 4, dst += 2) {
        dst[0] = (src[0] + src[2]) * 0.5f; /* left */
        dst[1] = (src[1] + src[3]) * 0.5f; /* right */
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in a quadraphonic format, while output
 * should support a tri-valued stereo stream (left-right-center)
 * with the same number of audio frames. The value size is specified
 * in terms of frames, not samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_quad_to_tri(const float* input, float* output, size_t size) {
    const float *src = input;
    float *dst = output;

    /* SDL's 5.1 layout: FL+FR+FC+LFE+BL+BR */
    for (size_t ii = 0; ii < size; ii++, src += 4, dst += 3) {
        dst[0] = (src[0] + src[2]) * 0.5f; /* left */
        dst[1] = (src[1] + src[3]) * 0.5f; /* right */
        dst[2] = (dst[0] + dst[1]) * 0.5f; /* center */
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in 5.1 surround, while output
 * should support a stereo stream with the same number of audio
 * frames. The value size is specified in terms of frames, not
 * samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_51_to_stereo(const float* input, float* output, size_t size) {
    const float *src = input;
    float *dst = output;

    /* SDL's 5.1 layout: FL+FR+FC+LFE+BL+BR */
    for (size_t ii = 0; ii < size; ii++, src += 6, dst += 2) {
        const float front_center_distributed = src[2] * 0.5f;
        dst[0] = (src[0] + front_center_distributed + src[4]) / 2.5f;  /* left */
        dst[1] = (src[1] + front_center_distributed + src[5]) / 2.5f;  /* right */
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in 5.1 surround, while output should
 * support a tri-valued stereo stream (left-right-center) with the
 * same number of audio frames. The value size is specified in
 * terms of frames, not samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_51_to_tri(const float* input, float* output, size_t size) {
    const float *src = input;
    float *dst = output;

    /* SDL's 5.1 layout: FL+FR+FC+LFE+BL+BR */
    for (size_t ii = 0; ii < size; ii++, src += 6, dst += 3) {
        dst[0] = (src[0] + src[4]) * 0.5f; /* left */
        dst[1] = (src[1] + src[5]) * 0.5f; /* right */
        dst[2] = src[2];                   /* center */
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in 5.1 surround, while output
 * should support a quadraphonic stream with the same number
 * of audio frames. The value size is specified in terms of
 * frames, not samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_51_to_quad(const float* input, float* output, size_t size) {
    const float *src = input;
    float *dst = output;

    /* SDL's 4.0 layout: FL+FR+BL+BR */
    /* SDL's 5.1 layout: FL+FR+FC+LFE+BL+BR */
    for (size_t ii = 0; ii < size; ii++, src += 6, dst += 4) {
        const float front_center_distributed = src[2] * 0.5f;
        dst[0] = (src[0] + front_center_distributed) / 1.5f;  /* FL */
        dst[1] = (src[1] + front_center_distributed) / 1.5f;  /* FR */
        dst[2] = src[4] / 1.5f;  /* BL */
        dst[3] = src[5] / 1.5f;  /* BR */
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in 7.1 surround, while output
 * should support a 5.1 surround stream with the same number
 * of audio frames. The value size is specified in terms of
 * frames, not samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_71_to_51(const float* input, float* output, size_t size) {
    const float *src = input;
    float *dst = output;

    for (size_t ii = 0; ii < size; ii++, src += 8, dst += 6) {
        const float surround_left_distributed = src[6] * 0.5f;
        const float surround_right_distributed = src[7] * 0.5f;
        dst[0] = (src[0] + surround_left_distributed)  / 1.5f;   /* FL */
        dst[1] = (src[1] + surround_right_distributed) / 1.5f;  /* FR */
        dst[2] = src[2] / 1.5f; /* CC */
        dst[3] = src[3] / 1.5f; /* LFE */
        dst[4] = (src[4] + surround_left_distributed)  / 1.5f;   /* BL */
        dst[5] = (src[5] + surround_right_distributed) / 1.5f;  /* BR */
    }
}

// The following have limitations on when they can be done in place
#pragma mark -
#pragma mark Upward Conversions
/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in mono, while output should
 * support a stereo stream with the same number of audio frames.
 * The value size is specified in terms of frames, not samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_mono_to_stereo(const float* input, float* output, size_t size) {
    const float *src = (input + size);
    float *dst = (output + size*2);

    for (size_t ii = 0; ii < size; ii++) {
        src--;
        dst -= 2;
        dst[0] = dst[1] = *src;
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in stereo, while output should
 * support a tri-valued stereo stream (left-right-center) with
 * the same number of audio frames. The value size is specified
 * in terms of frames, not samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_stereo_to_tri(const float* input, float* output, size_t size) {
    const float *src = (input + size*2);
    float *dst = (output + size*4);
    float lf, rf;

    for (size_t ii = 0; ii < size; ii++) {
        dst -= 3;
        src -= 2;
        lf = src[0];
        rf = src[1];
        dst[0] = lf;            /* FL */
        dst[1] = rf;            /* FR */
        dst[2] = (lf+rf)*0.5f;  /* FC */
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in stereo, while output should
 * support a quadraphonic stream with the same number of audio
 * frames. The value size is specified in terms of frames, not
 * samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_stereo_to_quad(const float* input, float* output, size_t size) {
    const float *src = (input + size*2);
    float *dst = (output + size*4);
    float lf, rf;

    for (size_t ii = 0; ii < size; ii++) {
        dst -= 4;
        src -= 2;
        lf = src[0];
        rf = src[1];
        dst[0] = lf;  /* FL */
        dst[1] = rf;  /* FR */
        dst[2] = lf;  /* BL */
        dst[3] = rf;  /* BR */
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in stereo, while output should
 * support a 5.1 surround stream with the same number of
 * audio frames. The value size is specified in terms of
 * frames, not samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_stereo_to_51(const float* input, float* output, size_t size) {
    const float *src = (input + size*2);
    float *dst = (output + size*6);
    float lf, rf;

    for (size_t ii = 0; ii < size; ii++) {
        dst -= 6;
        src -= 2;
        lf = src[0];
        rf = src[1];
        dst[0] = lf;            /* FL */
        dst[1] = rf;            /* FR */
        dst[2] = (lf+rf)*0.5f;  /* FC */
        dst[3] = 0;             /* LFE (only meant for special LFE effects) */
        dst[4] = lf;            /* BL */
        dst[5] = rf;            /* BR */
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in tri-valued stereo (left-right-center),
 * while output should support a quadraphonic stream with the same number
 * of audio frames. The value size is specified in terms of frames,
 * not samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_tri_to_quad(const float* input, float* output, size_t size) {
    const float *src = (input + size*3);
    float *dst = (output + size*4);
    float lf, rf, ce;

    for (size_t ii = 0; ii < size; ii++) {
        dst -= 4;
        src -= 3;
        lf = src[0];
        rf = src[1];
        ce = src[2]*0.5f;
        dst[0] = (lf + ce) / 1.5f;  /* FL */
        dst[1] = (rf + ce) / 1.5f;  /* FR */
        dst[2] = lf / 1.5f;         /* BL */
        dst[3] = rf / 1.5f;         /* BR */
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in tri-valued stereo (left-right-center),
 * while output should support a 5.1 surround stream with the same
 * number of audio frames. The value size is specified in terms of
 * frames, not samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_tri_to_51(const float* input, float* output, size_t size) {
    const float *src = (input + size*3);
    float *dst = (output + size*6);
    float lf, rf, ce;

    for (size_t ii = 0; ii < size; ii++) {
        dst -= 6;
        src -= 3;
        lf = src[0];
        rf = src[1];
        ce = src[2];
        dst[0] = lf;        /* FL */
        dst[1] = rf;        /* FR */
        dst[2] = ce;        /* FC */
        dst[3] = 0;         /* LFE (only meant for special LFE effects) */
        dst[4] = lf / 1.5f; /* BL */
        dst[5] = rf / 1.5f; /* BR */
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in a quadraphonic format, while output
 * should support a 5.1 surround stream with the same number of audio
 * frames. The value size is specified in terms of frames, not samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_quad_to_51(const float* input, float* output, size_t size) {
    const float *src = (input + size*4);
    float *dst = (output + size*6);
    float lf, rf, lb, rb;

    for (size_t ii = 0; ii < size; ii++) {
        dst -= 6;
        src -= 4;
        lf = src[0];
        rf = src[1];
        lb = src[2];
        rb = src[3];
        dst[0] = lf;            /* FL */
        dst[1] = rf;            /* FR */
        dst[2] = (lf+rf)*0.5f;  /* FC */
        dst[3] = 0;             /* LFE (only meant for special LFE effects) */
        dst[4] = lb;            /* BL */
        dst[5] = rb;            /* BR */
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should be in 5.1 surround, while output
 * should support a 7.1 surround stream with the same number
 * of audio frames. The value size is specified in terms of
 * frames, not samples.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
static void convert_51_to_71(const float* input, float* output, size_t size) {
    const float *src = (input + size*6);
    float *dst = (output + size*8);
    float lf, rf, lb, rb;

    for (size_t ii = 0; ii < size; ii++) {
        dst -= 8;
        src -= 6;
        lf = src[0];
        rf = src[1];
        lb = src[4];
        rb = src[5];
        dst[3] = src[3];        /* LFE */
        dst[2] = src[2];        /* FC */
        dst[7] = (lf+lb)*0.5f;  /* SR */
        dst[6] = (rf+rb)*0.5f;  /* SL */
        dst[5] = rb;            /* BR */
        dst[4] = lb;            /* BL */
        dst[1] = rf;            /* FR */
        dst[0] = lf;            /* FL */
    }
}

/**
 * Returns a pointer to the correct channel conversion algorithm
 *
 * @param insize    The number of input channels
 * @param outsize   The number of output channels
 *
 * @return a pointer to the correct channel conversion algorithm
 */
static std::function<void(const float*, float*, size_t)> select_algorithm(Uint8 insize, Uint8 outsize) {
    switch(insize) {
        case 1:
            switch (outsize) {
                case 1:
                    return nullptr;
                case 2:
                    return convert_mono_to_stereo;
                case 3:
                    return [=](const float* input, float* output, size_t size) {
                        convert_mono_to_stereo(input, output, size);
                        convert_stereo_to_tri(output, output, size);
                    };
                case 4:
                    return [=](const float* input, float* output, size_t size) {
                        convert_mono_to_stereo(input, output, size);
                        convert_stereo_to_quad(output, output, size);
                    };
                case 6:
                    return [=](const float* input, float* output, size_t size) {
                        convert_mono_to_stereo(input, output, size);
                        convert_stereo_to_51(output, output, size);
                    };
                case 8:
                    return [=](const float* input, float* output, size_t size) {
                        convert_mono_to_stereo(input, output, size);
                        convert_stereo_to_51(output, output, size);
                        convert_51_to_71(output, output, size);
                    };
                default:
                    CUAssertLog(false,
                                "Nonstandard output channel size %d requires an explicit matrix.",
                                outsize);
                    return nullptr;
            }
        case 2:
            switch (outsize) {
                case 1:
                    return convert_stereo_to_mono;
                case 2:
                    return nullptr;
                case 3:
                    return convert_stereo_to_tri;
                case 4:
                    return convert_stereo_to_quad;
                case 6:
                    return convert_stereo_to_51;
                case 8:
                    return [=](const float* input, float* output, size_t size) {
                        convert_stereo_to_51(input, output, size);
                        convert_51_to_71(output, output, size);
                    };
                default:
                    CUAssertLog(false,
                                "Nonstandard output channel size %d requires an explicit matrix.",
                                outsize);
                    return nullptr;
            }
        case 3:
            switch (outsize) {
                case 1:
                    return [=](const float* input, float* output, size_t size) {
                        convert_tri_to_stereo(input, output, size);
                        convert_stereo_to_mono(output, output, size);
                    };
                case 2:
                    return convert_tri_to_stereo;
                case 3:
                    return nullptr;
                case 4:
                    return convert_tri_to_quad;
                case 6:
                    return convert_tri_to_51;
                case 8:
                    return [=](const float* input, float* output, size_t size) {
                        convert_tri_to_51(input, output, size);
                        convert_51_to_71(output, output, size);
                    };
                default:
                    CUAssertLog(false,
                                "Nonstandard output channel size %d requires an explicit matrix.",
                                outsize);
                    return nullptr;
            }
        case 4:
            switch (outsize) {
                case 1:
                    return [=](const float* input, float* output, size_t size) {
                        convert_quad_to_stereo(input, output, size);
                        convert_stereo_to_mono(output, output, size);
                    };
                case 2:
                    return convert_quad_to_stereo;
                case 3:
                    return convert_quad_to_tri;
                case 4:
                    return nullptr;
                case 6:
                    return convert_quad_to_51;
                case 8:
                    return [=](const float* input, float* output, size_t size) {
                        convert_quad_to_51(input, output, size);
                        convert_51_to_71(output, output, size);
                    };
                default:
                    CUAssertLog(false,
                                "Nonstandard output channel size %d requires an explicit matrix.",
                                outsize);
                    return nullptr;
            }
        case 6:
            switch (outsize) {
                case 1:
                    return [=](const float* input, float* output, size_t size) {
                        convert_51_to_stereo(input, output, size);
                        convert_stereo_to_mono(output, output, size);
                    };
                case 2:
                    return convert_51_to_stereo;
                case 3:
                    return convert_51_to_tri;
                case 4:
                    return convert_51_to_quad;
                case 6:
                    return nullptr;
                case 8:
                    return convert_51_to_71;
                default:
                    CUAssertLog(false,
                                "Nonstandard output channel size %d requires an explicit matrix.",
                                outsize);
                    return nullptr;
            }
        case 8:
            switch (outsize) {
                case 1:
                    return [=](const float* input, float* output, size_t size) {
                        convert_71_to_51(input, output, size);
                        convert_51_to_stereo(output, output, size);
                        convert_stereo_to_mono(output, output, size);
                    };
                case 2:
                    return [=](const float* input, float* output, size_t size) {
                        convert_71_to_51(input, output, size);
                        convert_51_to_stereo(output, output, size);
                    };
                case 3:
                    return [=](const float* input, float* output, size_t size) {
                        convert_71_to_51(input, output, size);
                        convert_51_to_tri(output, output, size);
                    };
                case 4:
                    return [=](const float* input, float* output, size_t size) {
                        convert_71_to_51(input, output, size);
                        convert_51_to_quad(output, output, size);
                    };
                case 6:
                    return convert_71_to_51;
                case 8:
                    return nullptr;
                default:
                    CUAssertLog(false,
                                "Nonstandard output channel size %d requires an explicit matrix.",
                                outsize);
                    return nullptr;
            }
        default:
            CUAssertLog(false,
                        "Nonstandard input channel size %d requires an explicit matrix.",
                        insize);
    }
    return nullptr;
}

#pragma mark -
#pragma mark Constructors
/**
 * Creates a degenerate channel redistributor.
 *
 * The redistributor has no channels, so read options will do nothing. The node must
 * be initialized to be used.
 */
AudioRedistributor::AudioRedistributor() :
_matrix(nullptr),
_buffer(nullptr),
_conduits(0),
_pagesize(0),
_matsize(0) {
    _input = nullptr;
    _director = nullptr;
    _classname = "AudioRedistributor";

}

/**
 * Initializes the redistributor with default stereo settings
 *
 * The number of channels is two, for stereo output.  The sample rate is
 * the modern standard of 48000 HZ.
 *
 * These values determine the buffer the structure for all {@link read}
 * operations.  In addition, they also detemine whether this node can
 * serve as an input to other nodes in the audio graph.
 *
 * @return true if initialization was successful
 */
bool AudioRedistributor::init() {
    return AudioNode::init();
}

/**
 * Initializes the redistributor with the given number of channels and sample rate
 *
 * These values determine the buffer the structure for all {@link read}
 * operations.  In addition, they also detemine whether this node can
 * serve as an input to other nodes in the audio graph.
 *
 * @param channels  The number of audio channels
 * @param rate      The sample rate (frequency) in HZ
 *
 * @return true if initialization was successful
 */
bool AudioRedistributor::init(Uint8 channels, Uint32 rate) {
    return AudioNode::init(channels,rate);
}

/**
 * Initializes the redistributor with the given input and number of channels
 *
 * The node acquires the sample rate of the input, but uses the given number
 * of channels as its output channels. The redistributor will use the default
 * redistribution algorithm for the given number of channels.  If input is
 * nullptr, this method will fail.
 *
 * @param input     The audio node to redistribute
 * @param channels  The number of audio channels
 *
 * @return true if initialization was successful
 */
bool AudioRedistributor::init(const std::shared_ptr<AudioNode>& input, Uint8 channels) {
    if (input && AudioNode::init(channels,input->getRate())) {
        attach(input);
        return true;
    }
    return false;
}

/**
 * Initializes the redistributor with the given input and matrix
 *
 * The node acquires the sample rate of the input, but uses the given number
 * of channels as its output channels. The redistributor will use the given
 * matrix to redistribute the audio. If input is nullptr, this method will
 * fail.
 *
 * The matrix should be an MxN matrix in row major order, where N is the number
 * of input channels and M is the number of output channels. The provided matrix
 * will be copied.  This method will not acquire ownership of the given matrix.
 *
 * @param input     The audio node to redistribute
 * @param channels  The number of audio channels
 * @param matrix    The redistribution matrix
 *
 * @return true if initialization was successful
 */
bool AudioRedistributor::init(const std::shared_ptr<AudioNode>& input, Uint8 channels, const float* matrix) {
    if (input && AudioNode::init(channels,input->getRate())) {
        attach(input,matrix);
        return true;
    }
    return false;
}

/**
 * Disposes any resources allocated for this redistributor
 *
 * The state of the node is reset to that of an uninitialized constructor.
 * Unlike the destructor, this method allows the node to be reinitialized.
 */
void AudioRedistributor::dispose() {
    if (_booted) {
        AudioNode::dispose();
        _input = nullptr;
        _director = nullptr;
        _pagesize = 0;
        if (_matrix != nullptr) {
            free(_matrix);
            _matrix = nullptr;
        }
        if (_buffer != nullptr) {
            free(_buffer);
            _buffer = nullptr;
        }
    }
}

#pragma mark -
#pragma mark Audio Graph
/**
 * Attaches an audio node to this redistributor.
 *
 * The redistributor will use the use the default redistribution algorithm
 * for the current number of channels.
 *
 * @param node  The audio node to redistribute
 *
 * @return true if the attachment was successful
 */
bool AudioRedistributor::attach(const std::shared_ptr<AudioNode>& node) {
    if (!_booted) {
        CUAssertLog(_booted, "Cannot attach to an uninitialized audio node");
        return false;
    } else if (node == nullptr) {
        detach();
        return true;
    } else if (node->getRate() != _sampling) {
        CUAssertLog(false,"Input node has wrong sample rate: %d", node->getRate());
        return false;
    }
    
    setConduits(node->getChannels());
    std::atomic_exchange_explicit(&_input,node,std::memory_order_acq_rel);
    return true;
}

/**
 * Attaches an audio node to this redistributor.
 *
 * The redistributor will use the given matrix to redistribute the audio. The
 * matrix should be an MxN matrix in row major order, where N is the number
 * of input channels and M is the number of output channels. The provided matrix
 * will be copied. This method will not acquire ownership of the given matrix.
 *
 * @param node      The audio node to redistribute
 * @param matrix    The redistribution matrix
 *
 * @return true if the attachment was successful
 */
bool AudioRedistributor::attach(const std::shared_ptr<AudioNode>& node, const float* matrix) {
    if (!_booted) {
        CUAssertLog(_booted, "Cannot attach to an uninitialized audio node");
        return false;
    } else if (node == nullptr) {
        detach();
        return true;
    } else if (node->getRate() != _sampling) {
        CUAssertLog(false,"Input node has wrong sample rate: %d", node->getRate());
        return false;
    }
    
    setConduits(node->getChannels(),matrix);
    std::atomic_exchange_explicit(&_input,node,std::memory_order_acq_rel);
    return true;

}

/**
 * Detaches an audio node from this redistributor.
 *
 * If the method succeeds, it returns the audio node that was removed.
 *
 * @return  The audio node to detach (or null if failed)
 */
std::shared_ptr<AudioNode> AudioRedistributor::detach() {
    if (!_booted) {
        CUAssertLog(_booted, "Cannot detach from an uninitialized audio node");
        return nullptr;
    }
    
    std::shared_ptr<AudioNode> result = std::atomic_exchange_explicit(&_input,{},std::memory_order_release);
    return result;
}

/**
 * Sets the number of input channels for this redistributor.
 *
 * Normally this number is inferred from the whatever input node is attached
 * to the redistributor. If no node has yet been attached, this this method
 * returns 0 by default.
 *
 * However, changing this value may require that the underlying read buffer
 * be resized (particularly when the number of input channels is larger
 * than the number of output channels.). Hence by setting this value ahead
 * of time (and making sure that all attached input nodes match this size),
 * you can improve the performance of this filter.
 *
 * Assigning this value while there is still an attached audio node has undefined
 * behavior.
 *
 * @param number    The number of input channels for this redistributor.
 */
void AudioRedistributor::setConduits(Uint8 number) {
    Uint8 original = _conduits.load(std::memory_order_acquire);
    if (original != number) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        _matsize  = 0;
        _pagesize = 0;
        if (_matrix != nullptr) {
            free(_matrix);
            _matrix = nullptr;
        }
        if (_buffer != nullptr) {
            free(_buffer);
            _buffer = nullptr;
        }
        if (number > _channels) {
            _pagesize = AudioDevices::get()->getReadSize()*number+number;
            _buffer = (float*)malloc(_pagesize*sizeof(float));
        }
        
        _director = select_algorithm(number, _channels);
        _conduits = number;
    }
}

/**
 * Sets the number of input channels for this redistributor.
 *
 * Normally this number is inferred from the whatever input node is attached
 * to the redistributor. If no node has yet been attached, this this method
 * returns 0 by default.
 *
 * However, changing this value may require that the underlying read buffer
 * be resized (particularly when the number of input channels is larger
 * than the number of output channels.). Hence by setting this value ahead
 * of time (and making sure that all attached input nodes match this size),
 * you can improve the performance of this filter.
 *
 * This version of the method will also allow you to set the matrix at the
 * same time (so that it matches the number of input channels). The matrix
 * will be an MxN matrix in row major order, where N is the number of input
 * channels and M is the number of output channels. The provided matrix will
 * be copied. This method will not acquire ownership of the given matrix.
 *
 * Assigning this value while there is still an attached audio node has undefined
 * behavior.
 *
 * @param number    The number of input channels for this redistributor.
 */
void AudioRedistributor::setConduits(Uint8 number, const float* matrix) {
    Uint8 original = _conduits.load(std::memory_order_acquire);
    if (original != number) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        _matsize  = 0;
        _pagesize = 0;
        if (_matrix != nullptr) {
            free(_matrix);
            _matrix = nullptr;
        }
        if (_buffer != nullptr) {
            free(_buffer);
            _buffer = nullptr;
        }
        if (number > _channels) {
            _pagesize = AudioDevices::get()->getReadSize()*number;
            _buffer = (float*)malloc(_pagesize*sizeof(float));
        }
        
        Uint32 size = (number+1)*_channels;
        _matrix = (float*)malloc(size*sizeof(float));
        std::memcpy(_matrix,matrix,size*sizeof(float));

        if (number > _channels) {
            _director = [this](const float* input, float* output, size_t size) {
                            scaleDown(input,output,size);
                        };
        } else {
            _director = [this](const float* input, float* output, size_t size) {
                            scaleUp(input,output,size);
                        };
        }
        _matsize = size;
        _conduits = number;
    }
}

/**
 * Returns the current redistribution matrix for this redistributor.
 *
 * The matrix will be an MxN matrix in row major order, where N is the number
 * of input channels and M is the number of output channels. If the redistributor
 * is currently using a default redistribution algorithm (based on the number
 * of input channels), then this method will return nullptr.
 *
 * @return the current redistribution matrix for this redistributor.
 */
const float* const AudioRedistributor::getMatrix() {
    Uint32 size = _matsize.load(std::memory_order_acquire);
    if (size > 0) {
        std::unique_lock<std::mutex> lk(_buffmtex);
        return _matrix;
    }
    return nullptr;
}

/**
 * Sets the current redistribution matrix for this redistributor.
 *
 * The matrix will be an MxN matrix in row major order, where N is the number
 * of input channels and M is the number of output channels. The provided matrix
 * will be copied. This method will not acquire ownership of the given matrix.
 *
 * If the redistributor is currently using a default redistribution algorithm
 * (based on the number of input channels), then this method will return nullptr.
 *
 * @param matrix    The redistribution matrix
 */
void AudioRedistributor::setMatrix(const float* matrix) {
    std::unique_lock<std::mutex> lk(_buffmtex);
    Uint32 size = _conduits;
    size = (size+1)*_channels;
    _matsize = 0;
    if (_matrix != nullptr) {
        free(_matrix);
        _matrix = nullptr;
    }
    _matrix = (float*)malloc(size*sizeof(float));
    std::memcpy(_matrix,matrix,size*sizeof(float));
    _director = nullptr;
    _matsize = size;
}

#pragma mark -
#pragma mark Overriden Methods
/**
 * Reads up to the specified number of frames into the given buffer
 *
 * AUDIO THREAD ONLY: Users should never access this method directly.
 * The only exception is when the user needs to create a custom subclass
 * of this AudioNode.
 *
 * The buffer should have enough room to store frames * channels elements.
 * The channels are interleaved into the output buffer.
 *
 * This method will always forward the read position after reading. Reading
 * again may return different data.
 *
 * @param buffer    The read buffer to store the results
 * @param frames    The maximum number of frames to read
 *
 * @return the actual number of frames read
 */
Uint32 AudioRedistributor::read(float* buffer, Uint32 frames) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    Uint32 take = 0;
    if (input == nullptr || _paused.load(std::memory_order_relaxed)) {
        std::memset(buffer,0,frames*_channels*sizeof(float));
        take = frames;
    } else {
        std::unique_lock<std::mutex> lk(_buffmtex);
        // Prevent a subtle race
        if (_conduits != input->getChannels() || _director == nullptr) {
            std::memset(buffer,0,frames*_channels*sizeof(float));
            take = frames;
        } else {
            bool abort = false;
            while (take < frames && !abort) {
                Uint32 amt = frames;
                if (_buffer != nullptr) {
                    amt = _pagesize < frames-take ? _pagesize : frames-take;
                    amt = input->read(_buffer,amt);
                    _director(_buffer,_buffer,amt);
                    std::memcpy(buffer+take*_channels, _buffer, amt*_channels*sizeof(float));
                } else {
                    amt = input->read(buffer+take*_channels,amt);
                    _director(buffer+take*_channels,buffer+take*_channels,amt);
                }
                take += amt;
                abort = (amt == 0);
            }
        }
    }
    return take;
}
    
/**
 * Returns true if this audio node has no more data.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns true if there is no input node, indicating there is no data.
 *
 * An audio node is typically completed if it return 0 (no frames read) on
 * subsequent calls to {@link read()}.
 *
 * @return true if this audio node has no more data.
 */
bool AudioRedistributor::completed() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    return (input == nullptr || input->completed());
}

/**
 * Marks the current read position in the audio steam.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns false if there is no input node, indicating it is unsupported.
 *
 * This method is used by {@link reset()} to determine where to restore
 * the read position.
 *
 * @return true if the read position was marked.
 */
bool AudioRedistributor::mark() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->mark();
    }
    return false;
}

/**
 * Clears the current marked position.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns false if there is no input node, indicating it is unsupported.
 *
 * Clearing the mark in a player is equivelent to setting the mark at
 * the beginning of the audio asset.  Future calls to {@link reset()}
 * will return to the start of the audio stream.
 *
 * @return true if the read position was cleared.
 */
bool AudioRedistributor::unmark() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->unmark();
    }
    return false;
}

/**
 * Resets the read position to the marked position of the audio stream.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns false if there is no input node, indicating it is unsupported.
 *
 * If no mark is set, this will reset to the player to the beginning of
 * the audio sample.
 *
 * @return true if the read position was moved.
 */
bool AudioRedistributor::reset() {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->reset();
    }
    return false;
}

/**
 * Advances the stream by the given number of frames.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * This method only advances the read position, it does not actually
 * read data into a buffer.
 *
 * @param frames    The number of frames to advace
 *
 * @return the actual number of frames advanced; -1 if not supported
 */
Sint64 AudioRedistributor::advance(Uint32 frames) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->advance(frames);
    }
    return -1;
}
    
/**
 * Returns the current frame position of this audio node
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * The value returned will always be the absolute frame position regardless
 * of the presence of any marks.
 *
 * @return the current frame position of this audio node.
 */
Sint64 AudioRedistributor::getPosition() const {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->getPosition();
    }
    return -1;
}

/**
 * Sets the current frame position of this audio node.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * The value set will always be the absolute frame position regardless
 * of the presence of any marks.
 *
 * @param position  the current frame position of this audio node.
 *
 * @return the new frame position of this audio node.
 */
Sint64 AudioRedistributor::setPosition(Uint32 position) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->getPosition();
    }
    return -1;
}

/**
 * Returns the elapsed time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * The value returned is always measured from the start of the steam,
 * regardless of the presence of any marks.
 *
 * @return the elapsed time in seconds.
 */
double AudioRedistributor::getElapsed() const {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->getElapsed();
    }
    return -1;
}

/**
 * Sets the read position to the elapsed time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * The value returned is always measured from the start of the steam,
 * regardless of the presence of any marks.
 *
 * @param time  The elapsed time in seconds.
 *
 * @return the new elapsed time in seconds.
 */
double AudioRedistributor::setElapsed(double time) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->setElapsed(time);
    }
    return -1;
}

/**
 * Returns the remaining time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * The remaining time is duration from the current read position to the
 * end of the sample.  It is not effected by any fade-out.
 *
 * @return the remaining time in seconds.
 */
double AudioRedistributor::getRemaining() const {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->getRemaining();
    }
    return -1;
}

/**
 * Sets the remaining time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to the input node.  It
 * returns -1 if there is no input node, indicating it is unsupported.
 *
 * This method will move the read position so that the distance between
 * it and the end of the same is the given number of seconds.
 *
 * @param time  The remaining time in seconds.
 *
 * @return the new remaining time in seconds.
 */
double AudioRedistributor::setRemaining(double time) {
    std::shared_ptr<AudioNode> input = std::atomic_load_explicit(&_input,std::memory_order_relaxed);
    if (input) {
        return input->setRemaining(time);
    }
    return -1;
}

#pragma mark -
#pragma mark Matrix Redistribution

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should have {@link #getConduits()} channels
 * worth of data, while output should support {@link #getChannels()}
 * channels. However, this method is safe to call in-place,
 * provided that the buffer is large enough to support the
 * new output data.
 *
 * This version of the method assumes that output has more channels
 * than input. This distinction is necessary to support in-place
 * redistribution. The value size is specified in terms of frames,
 * not samples, so that it does not include any channel information.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
void AudioRedistributor::scaleUp(const float* input, float* output, size_t size) {
    Uint32 rows = _channels;
    Uint32 cols = _conduits;
    Uint32 work = rows*cols;
    
    const float* src = input + size*cols;
    float* dst = output + size*rows;
    
    for(size_t ii = 0; ii < size; ii++) {
        dst -= rows;
        src -= cols;
        
        for(Uint32 ii = 0; ii < rows; ii++) {
            float total = 0;
            for (Uint32 jj = 0; jj < cols; jj++) {
                total += _matrix[ii*cols+jj]*src[jj];
            }
            _matrix[work+ii] = total;
        }
        std::memcpy(dst, _matrix+work, rows*sizeof(float));
    }
}

/**
 * Redistribute from buffer input to buffer output
 *
 * The buffer input should have {@link #getConduits()} channels
 * worth of data, while output should support {@link #getChannels()}
 * channels. However, this method is safe to call in-place,
 * provided that the buffer is large enough to support the
 * new output data.
 *
 * This version of the method assumes that input has more channels
 * than output. This distinction is necessary to support in-place
 * redistribution. The value size is specified in terms of frames,
 * not samples, so that it does not include any channel information.
 *
 * @param input     The input buffer
 * @param output    The output buffer
 * @param size      The number of audio frames to process
 */
void AudioRedistributor::scaleDown(const float* input, float* output, size_t size) {
    Uint32 rows = _channels;
    Uint32 cols = _conduits;
    Uint32 work = rows*cols;
    
    const float* src = input;
    float* dst = output;
    
    for(size_t ii = 0; ii < size; ii++, dst += rows, src += cols) {
        for(Uint32 ii = 0; ii < rows; ii++) {
            float total = 0;
            for (Uint32 jj = 0; jj < cols; jj++) {
                total += _matrix[ii*cols+jj]*src[jj];
            }
            _matrix[work+ii] = total;
        }
        std::memcpy(dst, _matrix+work, rows*sizeof(float));
    }
}
