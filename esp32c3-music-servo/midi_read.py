import mido

# 定义舵机通道的数量（例如 8 个舵机）
SERVO_COUNT = 8


# MIDI 音符编号到音符名称的转换
def midi_note_to_name(note):
    NOTES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
    octave = (note // 12) - 1
    note_name = NOTES[note % 12]
    return f"{note_name}{octave}"


# 循环映射音符到舵机通道
def map_note_to_servo(note_name):
    NOTES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
    octave = int(note_name[-1])
    note_base = note_name[:-1]
    base_note_index = NOTES.index(note_base)
    return (base_note_index + (octave * len(NOTES))) % SERVO_COUNT


# 解析 MIDI 文件并生成舵机指令
def parse_midi_file(midi_file_path, output_file_path):
    try:
        midi = mido.MidiFile(midi_file_path)
        instructions = []

        for track in midi.tracks:
            time_counter = 0
            active_notes = {}
            for msg in track:
                time_counter += msg.time
                if msg.type == "note_on" and msg.velocity > 0:
                    note_name = midi_note_to_name(msg.note)
                    servo_channel = map_note_to_servo(note_name)
                    active_notes[msg.note] = time_counter
                elif msg.type == "note_off" or (
                    msg.type == "note_on" and msg.velocity == 0
                ):
                    if msg.note in active_notes:
                        start_time = active_notes.pop(msg.note)
                        duration = int(
                            (time_counter - start_time) * 1000 / midi.ticks_per_beat
                        )
                        note_name = midi_note_to_name(msg.note)
                        servo_channel = map_note_to_servo(note_name)
                        instructions.append((servo_channel, duration))

        with open(output_file_path, "w") as f:
            for servo_channel, duration in instructions:
                f.write(f"{servo_channel}:{duration}\n")
        print(f"指令已保存到 {output_file_path}")

    except FileNotFoundError:
        print(f"找不到文件: {midi_file_path}")
    except Exception as e:
        print(f"处理 MIDI 文件时出错: {e}")


# 示例用法
parse_midi_file("Laputa - Castle in the Sky - Laputa Theme.mid", "output_servo.txt")
