// Reads a HyperBlade disc image in the browser: an ISO, or a BIN with its CUE. Returns the same
// [path, file] pairs as the folder picker. A CUE's audio tracks become Data/Music/TrackNN.wav.
Module['hbReadDiscImage'] = async (files) => {
    const sectorSize = 2048;
    const byName = new Map(files.map((file) => [file.name.toUpperCase(), file]));
    const cueFile = files.find((file) => file.name.toUpperCase().endsWith('.CUE'));
    const tracks = [];

    if (cueFile) {
        let current = null;

        for (const line of (await cueFile.text()).split(/\r?\n/)) {
            const words = line.trim().match(/"[^"]*"|\S+/g) ?? [];
            const keyword = (words[0] ?? '').toUpperCase();

            if (keyword === 'FILE') {
                const name = words[1].replace(/"/g, '');
                current = { image: byName.get(name.split(/[\\/]/).pop().toUpperCase()), name };
            } else if (keyword === 'TRACK') {
                tracks.push({ number: Number(words[1]), mode: words[2].toUpperCase(), file: current, index: {} });
            } else if (keyword === 'INDEX' && tracks.length > 0) {
                const [minutes, seconds, frames] = words[2].split(':').map(Number);
                tracks.at(-1).index[Number(words[1])] = (minutes * 60 + seconds) * 75 + frames;
            }
        }

        const missing = tracks.find((track) => !track.file?.image);

        if (missing) {
            throw new Error(`pick ${missing.file?.name ?? 'the BIN'} with the CUE`);
        }
    } else {
        const image = files.find((file) => /\.(ISO|BIN|IMG)$/i.test(file.name));

        if (!image) {
            throw new Error('pick an ISO, or a BIN with its CUE');
        }

        // A raw BIN starts each sector with a sync pattern: 00, ten FFs, 00.
        const head = new Uint8Array(await image.slice(0, 12).arrayBuffer());
        const raw = head[0] === 0 && head[11] === 0 && head.subarray(1, 11).every((byte) => byte === 0xff);
        tracks.push({ number: 1, mode: raw ? 'MODE1/2352' : 'MODE1/2048', file: { image }, index: { 1: 0 } });
    }

    const data = tracks.find((track) => track.mode !== 'AUDIO');

    if (!data) {
        throw new Error('the image has no data track');
    }

    const rawSize = Number(data.mode.split('/')[1]);
    const header = { 'MODE1/2352': 16, 'MODE2/2352': 24 }[data.mode] ?? 0;
    const dataStart = data.index[1] * rawSize;

    // Reads a run of sectors' user data, dropping each raw sector's header and error correction.
    const read = async (lba, length) => {
        const count = Math.ceil(length / sectorSize);
        const start = dataStart + lba * rawSize;
        const bytes = new Uint8Array(await data.file.image.slice(start, start + count * rawSize).arrayBuffer());

        if (rawSize === sectorSize) {
            return bytes.subarray(0, length);
        }

        const out = new Uint8Array(count * sectorSize);

        for (let i = 0; i < count; i++) {
            out.set(bytes.subarray(i * rawSize + header, i * rawSize + header + sectorSize), i * sectorSize);
        }

        return out.subarray(0, length);
    };

    const volume = await read(16, sectorSize);

    if (String.fromCharCode(...volume.subarray(1, 6)) !== 'CD001') {
        throw new Error('the data track is not ISO 9660');
    }

    const view = (bytes) => new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
    const root = view(volume);
    const pairs = [];
    const pending = [{ path: '', lba: root.getUint32(156 + 2, true), size: root.getUint32(156 + 10, true) }];

    // Bounded so a corrupt image cannot loop forever.
    for (let visited = 0; pending.length > 0 && visited < 4096; visited++) {
        const directory = pending.shift();
        const bytes = await read(directory.lba, directory.size);
        const records = view(bytes);
        let offset = 0;

        while (offset < bytes.length) {
            const length = bytes[offset];

            // Records never cross a sector, so a zero length pads to the next one.
            if (length === 0) {
                offset = (Math.floor(offset / sectorSize) + 1) * sectorSize;
                continue;
            }

            const record = offset;
            offset += length;

            const nameLength = bytes[record + 32];

            // Names 0 and 1 are the directory itself and its parent.
            if (nameLength === 1 && bytes[record + 33] <= 1) {
                continue;
            }

            const rawName = String.fromCharCode(...bytes.subarray(record + 33, record + 33 + nameLength));
            const entry = {
                path: directory.path + rawName.replace(/;\d+$/, '').replace(/\.$/, ''),
                lba: records.getUint32(record + 2, true),
                size: records.getUint32(record + 10, true),
            };

            if (bytes[record + 25] & 2) {
                pending.push({ ...entry, path: `${entry.path}/` });
            } else {
                pairs.push([entry.path, { arrayBuffer: async () => (await read(entry.lba, entry.size)).slice().buffer }]);
            }
        }
    }

    for (const [i, track] of tracks.entries()) {
        if (track.mode !== 'AUDIO') {
            continue;
        }

        // A track ends where the next one in its file starts, its pregap included.
        const next = tracks[i + 1];
        const start = track.index[1] * 2352;
        let end = track.file.image.size;

        if (next?.file === track.file) {
            end = (next.index[0] ?? next.index[1]) * 2352;
        }

        // CD audio is 16-bit stereo PCM at 44.1 kHz, so a WAV header is all it needs.
        const wav = new DataView(new ArrayBuffer(44));
        const tag = (at, text) => [...text].forEach((char, j) => wav.setUint8(at + j, char.charCodeAt(0)));
        tag(0, 'RIFF');
        wav.setUint32(4, 36 + end - start, true);
        tag(8, 'WAVEfmt ');
        wav.setUint32(16, 16, true);
        wav.setUint16(20, 1, true);
        wav.setUint16(22, 2, true);
        wav.setUint32(24, 44100, true);
        wav.setUint32(28, 44100 * 4, true);
        wav.setUint16(32, 4, true);
        wav.setUint16(34, 16, true);
        tag(36, 'data');
        wav.setUint32(40, end - start, true);

        const name = `DATA/MUSIC/TRACK${String(track.number).padStart(2, '0')}.WAV`;
        pairs.push([name, new Blob([wav, track.file.image.slice(start, end)])]);
    }

    return pairs;
};
