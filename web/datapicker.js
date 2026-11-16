// The player's own HyperBlade folder, picked once and kept in IndexedDB. Nothing leaves the browser.
Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(() => {
    const store = '/hb';

    addRunDependency('hyperblade-data');
    FS.mkdir(store);
    FS.mount(IDBFS, { autoPersist: true }, store);

    const findExe = () => FS.readdir(store).find((name) => name.toUpperCase() === 'HYPERX.EXE');

    const start = () => {
        // The career file sits beside the executable, so it is linked in like the data.
        const career = `${store}/hyperblade_20XX_career.txt`;

        if (!FS.analyzePath(career).exists) {
            FS.writeFile(career, '');
        }

        for (const name of FS.readdir(store)) {
            if (name !== '.' && name !== '..') {
                FS.symlink(`${store}/${name}`, `/${name}`);
            }
        }

        removeRunDependency('hyperblade-data');
    };

    const panel = document.createElement('div');
    panel.style.cssText = 'position:fixed;inset:0;display:flex;flex-direction:column;align-items:center;justify-content:center;gap:12px;background:#111;color:#ddd;font:16px sans-serif;text-align:center;padding:16px';
    const button = 'padding:8px 16px;border:1px solid #888;cursor:pointer';
    panel.innerHTML = '<p>HyperBlade 20XX plays from your own copy of HyperBlade.<br>Pick the folder that holds HYPERX.EXE, or the disc image: an ISO, or a BIN with its CUE.<br>It stays in this browser.</p>'
        + `<label style="${button}">Choose folder<input class="folder" type="file" webkitdirectory hidden></label>`
        + `<label style="${button}">Choose disc image<input class="disc" type="file" accept=".iso,.cue,.bin,.img" multiple hidden></label>`
        + '<p class="status"></p>';

    const status = panel.querySelector('.status');

    // Each pair is a path inside the picked folder, its top folder's name removed, and the file.
    Module['hbLoadGameFiles'] = async (pairs) => {
        const wanted = pairs.filter(([path]) => path.toUpperCase() === 'HYPERX.EXE' || path.toUpperCase().startsWith('DATA/'));

        if (!wanted.some(([path]) => path.toUpperCase() === 'HYPERX.EXE')) {
            status.textContent = 'That folder has no HYPERX.EXE.';
            return;
        }

        for (const [index, [path, blob]] of wanted.entries()) {
            status.textContent = `Copying ${index + 1} of ${wanted.length}`;

            const target = `${store}/${path}`;
            FS.mkdirTree(target.slice(0, target.lastIndexOf('/')));
            FS.writeFile(target, new Uint8Array(await blob.arrayBuffer()));
        }

        status.textContent = 'Saving to this browser';

        FS.syncfs(false, (error) => {
            if (error) {
                console.warn('HyperBlade data not kept for next time', error);
            }

            panel.remove();
            start();
        });
    };

    panel.querySelector('.folder').addEventListener('change', (event) => {
        const pairs = [...event.target.files].map((file) => [file.webkitRelativePath.slice(file.webkitRelativePath.indexOf('/') + 1), file]);

        Module['hbLoadGameFiles'](pairs).catch((error) => {
            status.textContent = `Copying failed: ${error}`;
        });
    });

    panel.querySelector('.disc').addEventListener('change', (event) => {
        status.textContent = 'Reading the disc image';

        Module['hbReadDiscImage']([...event.target.files])
            .then((pairs) => Module['hbLoadGameFiles'](pairs))
            .catch((error) => {
                status.textContent = `Reading the disc image failed: ${error.message}`;
            });
    });

    FS.syncfs(true, () => {
        if (findExe()) {
            start();
            return;
        }

        document.body.append(panel);
    });
});
