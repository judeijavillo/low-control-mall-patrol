//
//  CUAudioMixer.h
//  Cornell University Game Library (CUGL)
//
//  Thie module provides an audio graph node for mixing together several input
//  streams into a single output stream. The input nodes must all have the same
//  same number of channels and sampling rate.
//
//  Mixing works by adding together all of the streams.  This means that the
//  results may exceed the range [-1,1], causing clipping.  The mixer provides
//  a "soft-knee" option for confining the results to the range [-1,1].
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
//  Version: 1/21/21
//
#ifndef __CU_AUDIO_MIXER_H__
#define __CU_AUDIO_MIXER_H__
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
 * This class represents an audio mixer.
 *
 * This mixer can take (a fixed number of) input streams and combine them
 * together into a single output stream.  The input streams must all have the
 * same number of channels and sample rate as this node.
 *
 * Mixing works by adding together all of the streams.  This means that the
 * results may exceed the range [-1,1], causing clipping.  The mixer provides
 * a "soft-knee" option (disabled by default) for confining the results to the
 * range [-1,1]. When a knee k is specified, all values [-k,k] will not be
 * effected, but values outside of this range will asymptotically bend to
 * the range [-1,1].
 *
 * The audio graph should only be accessed in the main thread.  In addition,
 * no methods marked as AUDIO THREAD ONLY should ever be accessed by the user.
 *
 * This class does not support any actions for the {@link AudioNode#setCallback}.
 */
class AudioMixer : public AudioNode {
private:
    /** The input nodes to be mixed */
    std::shared_ptr<AudioNode>* _inputs;
    /** The number of input nodes supported by this mixer */
    Uint8 _width;

    /** The intermediate buffer for the mixed result */
    float* _buffer;
    /** The capacity of the intermediate buffer */
    Uint32 _capacity;
    
    /** The knee value for clamping */
    std::atomic<float>  _knee;

    /** This class needs a proper lock guard; too many race conditions */
    std::mutex _mutex;
    /** The current read position */
    std::atomic<Uint64> _offset;
    /** The last marked position (starts at 0) */
    std::atomic<Uint64> _marked;

public:
#pragma mark Constructors
    /** The default number of inputs supported (typically 8) */
    static const Uint8 DEFAULT_WIDTH;
    /** The standard knee value for preventing clipping */
    static const float DEFAULT_KNEE;

    /**
     * Creates a degenerate mixer that takes no inputs
     *
     * The mixer has no width and therefore cannot accept any inputs. The mixer
     * must be initialized to be used.
     */
    AudioMixer();
    
    /**
     * Deletes this mixer, disposing of all resources.
     */
    ~AudioMixer() { dispose(); }
    
    /**
     * Initializes the mixer with default stereo settings
     *
     * The number of channels is two, for stereo output.  The sample rate is
     * the modern standard of 48000 HZ.
     *
     * These values determine the buffer the structure for all {@link read}
     * operations.  In addition, they also detemine exactly which audio nodes
     * are supported by this mixer.  A mixer can only mix nodes that agree
     * on both sample rate and frequency.
     *
     * @return true if initialization was successful
     */
    virtual bool init() override;

    /**
     * Initializes the mixer with default stereo settings
     *
     * The number of channels is two, for stereo output.  The sample rate is
     * the modern standard of 48000 HZ.
     *
     * These values determine the buffer the structure for all {@link read}
     * operations.  In addition, they also detemine exactly which audio nodes
     * are supported by this mixer.  A mixer can only mix nodes that agree
     * on both sample rate and frequency.
     *
     * @param width     The number of audio nodes that may be attached to this mixer
     *
     * @return true if initialization was successful
     */
    bool init(Uint8 width);

    /**
     * Initializes the mixer with the given number of channels and sample rate
     *
     * These values determine the buffer the structure for all {@link read}
     * operations.  In addition, they also detemine exactly which audio nodes
     * are supported by this mixer.  A mixer can only mix nodes that agree
     * on both sample rate and frequency.
     *
     * @param channels  The number of audio channels
     * @param rate      The sample rate (frequency) in HZ
     *
     * @return true if initialization was successful
     */
    virtual bool init(Uint8 channels, Uint32 rate) override;
    
    /**
     * Initializes the mixer with the given number of channels and sample rate
     *
     * These values determine the buffer the structure for all {@link read}
     * operations.  In addition, they also detemine exactly which audio nodes
     * are supported by this mixer.  A mixer can only mix nodes that agree
     * on both sample rate and frequency.
     *
     * @param width     The number of audio nodes that may be attached to this mixer
     * @param channels  The number of audio channels
     * @param rate      The sample rate (frequency) in HZ
     *
     * @return true if initialization was successful
     */
    bool init(Uint8 width, Uint8 channels, Uint32 rate);
    
    /**
     * Disposes any resources allocated for this mixer
     *
     * The state of the node is reset to that of an uninitialized constructor.
     * Unlike the destructor, this method allows the node to be reinitialized.
     */
    virtual void dispose() override;

#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a newly allocated mixer with default stereo settings
     *
     * The number of channels is two, for stereo output.  The sample rate is
     * the modern standard of 48000 HZ.
     *
     * These values determine the buffer the structure for all {@link read}
     * operations.  In addition, they also detemine exactly which audio nodes
     * are supported by this mixer.  A mixer can only mix nodes that agree
     * on both sample rate and frequency.
     *
     * @param width     The number of audio nodes that may be attached to this mixer
     *
     * @return a newly allocated mixer with default stereo settings
     */
    static std::shared_ptr<AudioMixer> alloc(Uint8 width=8) {
        std::shared_ptr<AudioMixer> result = std::make_shared<AudioMixer>();
        return (result->init(width) ? result : nullptr);
    }

    /**
     * Returns a newly allocated mixer with the given number of channels and sample rate
     *
     * These values determine the buffer the structure for all {@link read}
     * operations.  In addition, they also detemine exactly which audio nodes
     * are supported by this mixer.  A mixer can only mix nodes that agree
     * on both sample rate and frequency.
     *
     * @param width     The number of audio nodes that may be attached to this mixer
     * @param channels  The number of audio channels
     * @param rate      The sample rate (frequency) in HZ
     *
     * @return a newly allocated mixer with the given number of channels and sample rate
     */
    static std::shared_ptr<AudioMixer> alloc(Uint8 width, Uint8 channels, Uint32 rate) {
        std::shared_ptr<AudioMixer> result = std::make_shared<AudioMixer>();
        return (result->init(width,channels, rate) ? result : nullptr);
    }
    
#pragma mark -
#pragma mark Audio Graph Methods
    /**
     * Attaches an input node to this mixer.
     *
     * The input is attached at the given slot. Any input node previously at
     * that slot is removed (and returned by this method).
     *
     * @param slot  The slot for the input node
     * @param input The input node to attach
     *
     * @return the input node previously at the given slot
     */
    std::shared_ptr<AudioNode> attach(Uint8 slot, const std::shared_ptr<AudioNode>& input);
    
    /**
     * Detaches the input node at the given slot.
     *
     * The input node detached is returned by this method.
     *
     * @param slot  The slot for the input node
     *
     * @return the input node detached from the slot
     */
    std::shared_ptr<AudioNode> detach(Uint8 slot);
    
    /**
     * Returns true if this audio node has no more data.
     *
     * DELEGATED METHOD: This method delegates its call to **all** currently
     * attached input nodes.  It returns true if there are no attached input
     * nodes, or if **all** of the input nodes are complete.
     *
     * An audio node is typically completed if it return 0 (no frames read) on
     * subsequent calls to {@link read()}.
     *
     * @return true if this audio node has no more data.
     */
    virtual bool completed() override;
    
    /**
     * Reads up to the specified number of frames into the given buffer
     *
     * AUDIO THREAD ONLY: Users should never access this method directly, unless
     * part of a custom audio graph node.
     *
     * The buffer should have enough room to store frames * channels elements.
     * The channels are interleaved into the output buffer.
     *
     * Reading the buffer has no affect on the read position.  You must manually
     * move the frame position forward.  This is to allow for a frame window to
     * be reread if necessary.
     *
     * @param buffer    The read buffer to store the results
     * @param frames    The maximum number of frames to read
     *
     * @return the actual number of frames read
     */
    virtual Uint32 read(float* buffer, Uint32 frames) override;
    
    /**
     * Returns the width of this mixer.
     *
     * The width is the number of supported input slots.
     *
     * @return the width of this mixer.
     */
    Uint8 getWidth() const { return _width; }

    /**
     * Sets the width of this mixer.
     *
     * The width is the number of supported input slots. This method will only
     * succeed if the mixer is paused.  Otherwise, it will fail.
     *
     * Once the width is adjusted, the children will be reassigned in order.
     * If the new width is less than the old width, children at the end of
     * the mixer will be dropped.
     *
     * @return true if the mixer width was reset
     */
    bool setWidth(Uint8 width);

#pragma mark -
#pragma mark Anticlipping Methods
    /**
     * Returns the "soft knee" of this mixer, or -1 if not set
     *
     * The soft knee is used to ensure that the results fit in the range [-1,1].
     * If the knee is k, then values in the range [-k,k] are unaffected, but
     * values outside of this range are asymptotically clamped to the range
     * [-1,1], using the formula (x-k+k*k)/x.
     *
     * If the value is 0, then this mixer will hard clamp to [-1,1]. If it is
     * negative, all inputs will be mixed exactly with no distortion.
     *
     * @return the "soft knee" of this mixer, or -1 if not set
     */
    float getKnee() const;
    
    /**
     * Sets the "soft knee" of this mixer.
     *
     * The soft knee is used to ensure that the results fit in the range [-1,1].
     * If the knee is k, then values in the range [-k,k] are unaffected, but
     * values outside of this range are asymptotically clamped to the range
     * [-1,1], using the formula (x-k+k*k)/x
     *
     * If the value is 0, then this mixer will hard clamp to [-1,1]. If it is
     * negative, all inputs will be mixed exactly with no distortion.
     *
     * @param knee  the "soft knee" of this mixer
     */
    void setKnee(float knee);

#pragma mark -
#pragma mark Optional Methods
    /**
     * Marks the current read position in the audio steam.
     *
     * DELEGATED METHOD: This method delegates its call to **all** currently
     * attached input nodes.  It returns false if there are no attached input
     * nodes, or if just **one** of the input nodes does not support marking.
     * However, any input that was successfully marked remains marked.
     *
     * This method is typically used by {@link #reset()} to determine where to
     * restore the read position. For some nodes (like {@link AudioInput}),
     * this method may start recording data to a buffer, which will continue
     * until {@link #reset()} is called.
     *
     * Input nodes added to the mixer after this method is called are not
     * affected.
     *
     * @return true if the read position was marked across all inputs.
     */
    virtual bool mark() override;
    
    /**
     * Clears the current marked position.
     *
     * DELEGATED METHOD: This method delegates its call to **all** currently
     * attached input nodes.  It returns false if there are no attached input
     * nodes, or if just **one** of the input nodes does not support marking.
     * However, any input that was successfully unmarked remains unmarked.
     *
     * If the method {@link #mark()} started recording to a buffer (such as
     * with {@link AudioInput}), this method will stop recording and release
     * the buffer.  When the mark is cleared, {@link #reset()} may or may not
     * work depending upon the specific node.
     *
     * This method is ideal for a mixer composed of {@link AudioPlayer}
     * objects. It will equally mark all of the components, keeping them in
     * sync.
     *
     * @return true if the read position was marked.
     */
    virtual bool unmark() override;
    
    /**
     * Resets the read position to the marked position of the audio stream.
     *
     * DELEGATED METHOD: This method delegates its call to **all** currently
     * attached input nodes.  It returns false if there are no attached input
     * nodes, or if just **one** of the input nodes cannot be reset. However,
     * However, any input that was successfully reset remains reset.
     *
     * This method is ideal for a mixer composed of {@link AudioPlayer}
     * objects. It will equally unmark all of the components, keeping them
     * in sync.
     *
     * @return true if the read position was moved.
     */
    virtual bool reset() override;
    
    /**
     * Advances the stream by the given number of frames.
     *
     * DELEGATED METHOD: This method delegates its call to **all** currently
     * attached input nodes.  It returns -1 if there are no attached input
     * nodes, or if just **one** of the input nodes cannot be advanced.
     * However, any input that was successfully advanced remains advanced.
     *
     * This method is ideal for a mixer composed of {@link AudioPlayer}
     * objects. It will equally reset all of the components, keeping them in
     * sync.
     *
     * @param frames    The number of frames to advace
     *
     * @return the actual number of frames advanced; -1 if not supported
     */
    virtual Sint64 advance(Uint32 frames) override;
    
    /**
     * Returns the current frame position of this audio node
     *
     * If {@link #mark()} was called previously, this value is the number
     * of frames since the mark. Otherwise, it is the number of frames
     * since the start of this stream. Calling {@link #reset()} will
     * reset this position even if not all of the inputs were reset.
     *
     * This method is ideal for a mixer composed of {@link AudioPlayer}
     * objects. In that case, it will return the synchronous position
     * of all of the players.
     *
     * @return the current frame position of this audio node.
     */
    virtual Sint64 getPosition() const override;
    
    /**
     * Sets the current frame position of this audio node.
     *
     * DELEGATED METHOD: This method delegates its call to **all** currently
     * attached input nodes.  It returns -1 if there are no attached input
     * nodes, or if just **one** of the input nodes cannot be repositioned.
     * However, any input that was successfully repositioned remains
     * repositioned.
     *
     * This method is ideal for a mixer composed of {@link AudioPlayer}
     * objects. In that case, it will set the synchronous position
     * of all of the players.
     *
     * @param position  the current frame position of this audio node.
     *
     * @return the new frame position of this audio node.
     */
    virtual Sint64 setPosition(Uint32 position) override;
    
    /**
     * Returns the elapsed time in seconds.
     *
     * If {@link #mark()} was called previously, this value is the number
     * of seconds since the mark. Otherwise, it is the number of seconds
     * since the start of this stream. Calling {@link #reset()} will
     * reset this position even if not all of the inputs were reset.
     *
     * This method is ideal for a mixer composed of {@link AudioPlayer}
     * objects. In that case, it will return the synchronous elapsed
     * time of all of the players.
     *
     * @return the elapsed time in seconds.
     */
    virtual double getElapsed() const override;
    
    /**
     * Sets the read position to the elapsed time in seconds.
     *
     * DELEGATED METHOD: This method delegates its call to **all** currently
     * attached input nodes.  It returns -1 if there are no attached input
     * nodes, or if just **one** of the input nodes cannot be repositioned.
     * However, any input that was successfully repositioned remains
     * repositioned.
     *
     * This method is ideal for a mixer composed of {@link AudioPlayer}
     * objects. In that case, it will set the synchronous position
     * of all of the players.
     *
     * @param time  The elapsed time in seconds.
     *
     * @return the new elapsed time in seconds.
     */
    virtual double setElapsed(double time) override;
    
    /**
     * Returns the remaining time in seconds.
     *
     * DELEGATED METHOD: This method delegates its call to **all** currently
     * attached input nodes. The value returned is the maximum value across
     * all nodes. It returns -1 if there are no attached input node, or if
     * **any** attached node does not support this method.
     *
     * This method is ideal for a mixer composed of {@link AudioPlayer}
     * objects. In that case, it will return the maximum remaining time
     * across all of the players.
     *
     * @return the remaining time in seconds.
     */
    virtual double getRemaining() const override;
    
    /**
     * Sets the remaining time in seconds.
     *
     * DELEGATED METHOD: This method delegates its call to **all** currently
     * attached input nodes. The value set is the relative to the maximum
     * remaining time across all inputs. Any input with less than the remaining
     * time is advanced forward so that it remains in sync with the maximal
     * input.
     *
     * This method returns  returns -1 if there are no attached input node,
     * or if **any** attached node does not support this method. However,
     * any repositioned input will remain repositioned.
     *
     * This method is ideal for a mixer composed of {@link AudioPlayer}
     * objects. In that case, it will set the synchronous position
     * of all of the players.
     *
     * @param time  The remaining time in seconds.
     *
     * @return the new remaining time in seconds.
     */
    virtual double setRemaining(double time) override;
    
};
    }
}

#endif /* __CU_AUDIO_MIXER_H__ */
