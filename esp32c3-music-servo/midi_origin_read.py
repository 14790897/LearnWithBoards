import mido


# 读取并打印 MIDI 文件的所有信息
def read_midi_file(midi_file_path):
    # 打开 MIDI 文件
    midi = mido.MidiFile(midi_file_path)

    # 打印 MIDI 文件的基本信息
    print(f"文件类型: {midi.type}")
    print(f"每拍的 ticks 数: {midi.ticks_per_beat}")
    print(f"总轨道数: {len(midi.tracks)}\n")

    # 遍历每个音轨
    for i, track in enumerate(midi.tracks):
        print(f"音轨 {i}: {track.name}")
        print("-" * 40)

        # 遍历音轨中的每条消息
        for msg in track:
            print(msg)

        print("\n")


# 示例用法
read_midi_file("Laputa - Castle in the Sky - Laputa Theme.mid")
