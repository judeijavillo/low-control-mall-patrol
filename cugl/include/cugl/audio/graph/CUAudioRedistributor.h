//
//  CUAudioRedistributor.h
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
#ifndef __CU_AUDIO_REDISTRIBUTOR_H__
#define __CU_AUDIO_REDISTRIBUTOR_H__
#include <SDL/SDL.h>
#include "CUAudioNode.h"
#include <mutex>

namespace cugl {

    /**
     * The audio graph classes.
     *
     * This internal namespace is for the audio graph clases.  It was chosen
     * to distinguish this graph from other graph class collections, such as the
     * scene graph collections in {@link scene2}.
     */
    namespace audio {

/**
 * This class represents an audio channel redistributor.
 *
 * A channel redistributor is a node whose input number of channels does not
 * match the output number of channels. This is necessary for some latops, where
 * the output sound card requires more than two channels, even though the vast
 * majority of applications are designed for stereo (not surround) sound.
 *
 * A channel redistributor works by using a matrix to redistribute the input
 * channels, in much the same way that a matrix decoder works. However, unlike
 * a matrix decoder, it is possible to use a redistributor to reduce the number
 * of channels (with a matrix whose rows are less that is columns). Furthermore,
 * a redistributor does not support phase shifting.
 *
 * The audio graph should only be accessed in the main thread.  In addition,
 * no methods marked as AUDIO THREAD ONLY should ever be accessed by the user.
 *
 * This class does not support any actions for the {@link AudioNode#setCallback}.
 */
class AudioRedistributor : public AudioNode {
private:
    /** Mutex to protect more sophisticated synchronization */
    mutable std::mutex _buffmtex;

    /** The audio input node */
    std::shared_ptr<AudioNode> _input;
    /** The currently supported input channels size */
    std::atomic<Uint8> _conduits;

    /** The distribution function for default behavior */
    std::function<void(const float*, float*, size_t)> _director;
    
    /** The redistribution matrix (may be nullptr) */
    float* _matrix;
    /** The size of the redistribution matrix (may be 0) */
    std::atomic<Uint32> _matsize;
    
    /** An intermediate buffer for downscaling (may be nullptr) */
    float* _buffer;
    /** The capacity of the buffer */
    Uint32 _pagesize;
    
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
    void scaleUp(const float* input, float* output, size_t size);
    
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
    void scaleDown(const float* input, float* output, size_t);

public:
#pragma mark -
#pragma mark Constructors
    /**
     * Creates a degenerate channel redistributor.
     *
     * The redistributor has no channels, so read options will do nothing. The node must
     * be initialized to be used.
     */
    AudioRedistributor();
    
    /**
     * Deletes this redistributor, disposing of all resources.
     */
    ~AudioRedistributor() { dispose(); }
    
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
    virtual bool init() override;
    
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
    virtual bool init(Uint8 channels, Uint32 rate) override;
    
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
    bool init(const std::shared_ptr<AudioNode>& input, Uint8 channels);

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
    bool init(const std::shared_ptr<AudioNode>& input, Uint8 channels, const float* matrix);

    /**
     * Disposes any resources allocated for this redistributor
     *
     * The state of the node is reset to that of an uninitialized constructor.
     * Unlike the destructor, this method allows the node to be reinitialized.
     */
    virtual void dispose() override;
    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a newly allocated redistributor with default stereo settings
     *
     * The number of channels is two, for stereo output.  The sample rate is
     * the modern standard of 48000 HZ.
     *
     * These values determine the buffer the structure for all {@link read}
     * operations.  In addition, they also detemine whether this node can
     * serve as an input to other nodes in the audio graph.
     *
     * @return a newly allocated redistributor with default stereo settings
     */
    static std::shared_ptr<AudioRedistributor> alloc() {
        std::shared_ptr<AudioRedistributor> result = std::make_shared<AudioRedistributor>();
        return (result->init() ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated redistributor with the given number of channels and sample rate
     *
     * These values determine the buffer the structure for all {@link read}
     * operations.  In addition, they also detemine whether this node can
     * serve as an input to other nodes in the audio graph.
     *
     * @param channels  The number of audio channels
     * @param rate      The sample rate (frequency) in HZ
     *
     * @return a newly allocated redistributor with the given number of channels and sample rate
     */
    static std::shared_ptr<AudioRedistributor> alloc(Uint8 channels, Uint32 rate) {
        std::shared_ptr<AudioRedistributor> result = std::make_shared<AudioRedistributor>();
        return (result->init(channels,rate) ? result : nullptr);
    }

    /**
     * Returns a newly allocated redistributor with the given input and number of channels
     *
     * The node acquires the sample rate of the input, but uses the given number
     * of channels as its output channels. The redistributor will use the default
     * redistribution algorithm for the given number of channels.  If input is
     * nullptr, this method will fail.
     *
     * @param input     The audio node to redistribute
     * @param channels  The number of audio channels
     *
     * @return a newly allocated redistributor with the given input and number of channels
     */
    static std::shared_ptr<AudioRedistributor> alloc(const std::shared_ptr<AudioNode>& input, Uint8 channels) {
        std::shared_ptr<AudioRedistributor> result = std::make_shared<AudioRedistributor>();
        return (result->init(input,channels) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated redistributor with the given input and matrix
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
     * @return a newly allocated redistributor with the given input and matrix
     */
    static std::shared_ptr<AudioRedistributor> alloc(const std::shared_ptr<AudioNode>& input,
                                                     Uint8 channels, const float* matrix) {
        std::shared_ptr<AudioRedistributor> result = std::make_shared<AudioRedistributor>();
        return (result->init(input,channels,matrix) ? result : nullptr);
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
    bool attach(const std::shared_ptr<AudioNode>& node);
    
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
    bool attach(const std::shared_ptr<AudioNode>& node, const float* matrix);
    
    /**
     * Detaches an audio node from this redistributor.
     *
     * If the method succeeds, it returns the audio node that was removed.
     *
     * @return  The audio node to detach (or null if failed)
     */
    std::shared_ptr<AudioNode> detach();
    
    /**
     * Returns the input node of this redistributor.
     *
     * @return the input node of this redistributor.
     */
    std::shared_ptr<AudioNode> getInput() { return _input; }
    
    /**
     * Returns the number of input channels for this redistributor.
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
     * @return the number of input channels for this redistributor.
     */
    Uint8 getConduits() const { return _conduits.load(std::memory_order_relaxed); }
    
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
    void setConduits(Uint8 number);
    
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
     * @param matrix    The redistribution matrix
     */
    void setConduits(Uint8 number, const float* matrix);
    
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
    const float* const getMatrix();
    
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
    void setMatrix(const float* matrix);

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
    virtual Uint32 read(float* buffer, Uint32 frames) override;
    
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
    virtual bool completed() override;
    
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
    virtual bool mark() override;
    
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
    virtual bool unmark() override;
    
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
    virtual bool reset() override;
    
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
    virtual Sint64 advance(Uint32 frames) override;
    
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
    virtual Sint64 getPosition() const override;
    
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
    virtual Sint64 setPosition(Uint32 position) override;
    
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
    virtual double getElapsed() const override;
    
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
    virtual double setElapsed(double time) override;
    
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
    virtual double getRemaining() const override;
    
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
    virtual double setRemaining(double time) override;
};
    
    }
}
#endif /* __CU_AUDIO_REDISTRIBUTOR_H__ */
