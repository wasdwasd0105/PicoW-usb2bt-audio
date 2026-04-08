# Apple AAC-ELD over A2DP (Vendor Codec 0x8001)


## Overview

Apple uses AAC-ELD as a **vendor-specific A2DP codec** (not standard MPEG-2/4 AAC) for streaming audio to AirPods. It is registered under Apple's Bluetooth vendor ID `0x004C` with codec ID `0x8001`. Despite being transmitted over A2DP (media channel), the codec is internally classified as **UWBS (Ultra-Wideband Speech)** in Apple's Bluetooth stack, alongside mSBC and CVSD.

## AVDTP Codec Discovery

### SEP Capability (14 bytes)

AirPods advertises AAC-ELD as a non-A2DP vendor codec in its AVDTP SEP capabilities:

```
4C 00 00 00   vendor_id = 0x0000004C (Apple, little-endian)
01 80         codec_id  = 0x8001 (AAC-ELD, little-endian)
00 80         object_type bitmap = 0x0080 (AAC-ELD)
00 8C         sampling_freq bitmap (0x008 = 48kHz) | channels (0xC = mono + stereo)
00            reserved
83 E8 00      VBR=1 (bit 23), max bitrate = 256000 (bits 22..0)
```

### SET_CONFIGURATION (14 bytes)

The source sends back the selected configuration:

```
4C 00 00 00   vendor_id = 0x0000004C
01 80         codec_id  = 0x8001
00 80         object_type = AAC-ELD
00 84         freq = 48kHz (0x008) | channels = stereo (0x4)
00            reserved
83 E8 00      VBR=1, bitrate = 256000
```

## Encoder Configuration

macOS uses the Apple's AAC encoder with the following parameters:

| Parameter | Value | Notes |
|-----------|-------|-------|
| **AOT** | 39 (ER AAC ELD) | Error-Resilient AAC Enhanced Low Delay |
| **Sample Rate** | 48000 Hz | Input and output |
| **Channels** | 2 (Stereo) | |
| **Granule Length** | 480 samples | 10ms per frame at 48kHz |
| **SBR** | Enabled (downsampled, ratio=1) | `AACEnhancedLowDelaySBREncoder` active |
| **Bitrate** | ~227 kbps (measured) | CBR mode |
| **Transport** | Raw Access Units | No LATM/LOAS/ADTS framing |
| **Frame Rate** | ~100 fps | 48000 / 480 = 100 frames/sec |

### Mode Identification

Apple internally identifies this configuration as mode **130 = AAC-ELD-Stereo48K-10ms**:
- Mode 129 = AAC-ELD-Stereo48K (512 samples, ~10.67ms)
- Mode 130 = AAC-ELD-Stereo48K-10ms (480 samples, exactly 10ms)

### SBR Details

macOS uses ELD with SBR at ratio=1 (downsampled SBR), meaning the SBR extension operates at the same sample rate as the core coder (48kHz). This differs from HE-AAC's dual-rate SBR (ratio=2). The system_profiler shows `Current SampleRate: 24000` because the transport/core rate is half, with SBR reconstructing the upper band.

### Measured Frame Characteristics

From dtrace capture of `AudioCodecs::AACEncoder::AACEncodeOutput`:

```
Total frames captured: 299 (in 3.19 seconds)
Frame rate: 93.8 fps (48000/512 effective with SBR)
Frame sizes: min=278, max=320, avg=303 bytes
Average bitrate: ~227 kbps
First byte distribution: 0x85 (90%), 0x86 (8%), 0x84 (2%)
```

The first byte of each raw AU is the AAC `global_gain` field.



## RTP Packet Format

### RTP Header (12 bytes, standard)

Standard A2DP RTP header with media payload type for vendor codec.

### RTP Payload Structure

Each RTP packet contains **3 AAC-ELD frames**, each preceded by a 4-byte Apple header:

```
+-- Frame 1 ----------------------------------------+
| Apple Header (4 bytes) | Raw AAC-ELD AU (~300 bytes) |
+-- Frame 2 ----------------------------------------+
| Apple Header (4 bytes) | Raw AAC-ELD AU (~300 bytes) |
+-- Frame 3 ----------------------------------------+
| Apple Header (4 bytes) | Raw AAC-ELD AU (~300 bytes) |
+---------------------------------------------------+

Total payload: ~912 bytes (3 * (4 + ~300))
```

### Apple AAC-ELD Size Header (4 bytes per frame)

```
Byte 0: 0xB0 | (sequence >> 8) & 0x0F    -- marker 0xB0 + seq high 4 bits
Byte 1: sequence & 0xFF                    -- seq low 8 bits
Byte 2: 0x10 | (au_size >> 8) & 0x0F      -- marker 0x10 + size high 4 bits
Byte 3: au_size & 0xFF                     -- size low 8 bits
```

- **Sequence**: 12-bit counter (0-4095), increments per frame, wraps at 0xFFF
- **AU Size**: 12-bit field, max 2047 bytes (0x7FF). BTAudioHALPlugin logs `FATAL: Unable to correctly create AAC-ELD Size Header for %d bytes of encoded audio (max: 0x7FF = 2047)` if exceeded
- **Marker nibbles**: 0xB (byte 0 high) and 0x1 (byte 2 high) are fixed identifiers

Example: `B0 0A 11 33` = sequence 10, AU size 307 bytes

### RTP Timestamp

Sample-count based (standard A2DP), incremented by `frames_sent * 480` per packet:
- 3 frames per packet: timestamp += 1440
- Packet interval: 30ms (3 * 10ms frames)

## Packet Timing

| Metric | Value |
|--------|-------|
| Frames per packet | 3 |
| Samples per frame | 480 |
| Samples per packet | 1440 |
| Packet interval | 30ms |
| Packets per second | ~33 |
| Payload per packet | ~912 bytes |
| Effective bitrate (with headers) | ~250 kbps |

## Compatibility Notes

### What Works

- **AOT 39 (ER AAC ELD), no SBR, 480 samples, 265kbps CBR** with Apple per-frame header produces stable audio on AirPods

### What Doesn't Work

- **512-sample granule length**: AirPods plays ~0.1s then stops. Must be 480 (10ms mode)
- **No Apple header (raw AU concatenation)**: No audio output
- **SBR**: unstable

### FDK-AAC Encoder Settings (Working)

```c
aacEncoder_SetParam(handleAAC, AACENC_AOT, 39);           // ER AAC ELD
aacEncoder_SetParam(handleAAC, AACENC_BITRATE, 265000);   // 265kbps CBR
aacEncoder_SetParam(handleAAC, AACENC_SAMPLERATE, 48000);
aacEncoder_SetParam(handleAAC, AACENC_CHANNELMODE, 2);    // Stereo
aacEncoder_SetParam(handleAAC, AACENC_GRANULE_LENGTH, 480); // 10ms frames
aacEncoder_SetParam(handleAAC, AACENC_SBR_MODE, 0);       // SBR off
aacEncoder_SetParam(handleAAC, AACENC_BITRATEMODE, 0);    // CBR
aacEncoder_SetParam(handleAAC, AACENC_AFTERBURNER, 0);    // Off (CPU constraint)
aacEncoder_SetParam(handleAAC, AACENC_TRANSMUX, TT_MP4_RAW); // Raw AUs
```

## Signal Loss Recovery

AAC-ELD uses inter-frame prediction (overlap-add windowing), so both the encoder and decoder maintain internal state that depends on previous frames. When Bluetooth signal is temporarily lost (e.g. AirPods out of range for 1-2 seconds), frames are dropped and the decoder state diverges from the encoder state. Simply resuming transmission produces silence — the decoder cannot decode frames whose overlap context it missed.

### Symptoms

When signal degrades, the L2CAP send buffer fills up and `can_send_now` callback stops firing. The audio timer continues encoding frames, but `codec_ready_to_send` stays 1 (stuck). After signal returns, packets can be sent again, but AirPods produces no audio despite receiving data.

### Recovery: SUSPEND → START with Encoder Reset

The only reliable recovery is a full AVDTP SUSPEND → START cycle, which tells AirPods to flush and reinitialize its decoder. Combined with a local encoder reset, both sides start from a clean state.

On detecting a send failure (100ms without `can_send_now`):

1. **Reset the FDK-AAC encoder** — close and reopen with the same parameters to flush internal overlap/prediction buffers
2. **Reset `aaceld_frame_sequence` to 1** — fresh 12-bit sequence counter
3. **Reset `rtp_timestamp` to 0** — avoid timestamp discontinuity
4. **Send AVDTP SUSPEND** — AirPods resets its decoder
5. **On SUSPEND confirmation, send AVDTP START** — stream resumes with clean state on both sides

```c
// Trigger on first send failure for AAC-ELD (before signal returns on its own)
int suspend_threshold = (cur_codec == 4) ? 1 : 3;

if (fail_count >= suspend_threshold) {
    if (cur_codec == 4) {
        // Reset encoder state
        aacEncClose(&handleAAC);
        aacEncOpen(&handleAAC, 0x01, 2);
        // ... re-apply all AACENC_* params ...
        aacEncEncode(handleAAC, NULL, NULL, NULL, NULL);

        aaceld_frame_sequence = 1;
        context->rtp_timestamp = 0;
    }

    a2dp_demo_timer_stop(context);
    avdtp_source_suspend(cid, seid);
    // On AVDTP_SI_SUSPEND callback → avdtp_source_start_stream()
    // On AVDTP_SI_START callback → a2dp_demo_timer_start()
}
```
