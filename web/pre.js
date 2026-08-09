Module.preRun = Module.preRun || [];
Module.preRun.push(function () {
    var released = false;

    var exists = function (path) {
        try {
            FS.stat(path);
            return true;
        } catch (e) {
            return false;
        }
    };

    var release = function (err) {
        if (released) return;
        released = true;

        if (err) console.warn('A mentesek betoltese nem sikerult:', err);

        try {
            if (!exists('/saves/rang.lst')) FS.writeFile('/saves/rang.lst', '');
            if (!exists('/rang.lst')) FS.symlink('/saves/rang.lst', '/rang.lst');
        } catch (e) {
            console.warn('A ranglista nem elerheto:', e);
        }

        Module.removeRunDependency('persist');
    };

    Module.addRunDependency('persist');

    try {
        if (!exists('/saves')) FS.mkdir('/saves');
        FS.mount(IDBFS, {}, '/saves');
    } catch (e) {
        console.warn('A tartos tarolas nem erheto el, a mentesek nem maradnak meg:', e);
        release();
        return;
    }

    setTimeout(release, 3000);
    FS.syncfs(true, release);
});

Module.postRun = Module.postRun || [];
Module.postRun.push(function () {
    var flushing = false;
    var flush = function () {
        if (flushing) return;
        flushing = true;
        FS.syncfs(false, function (err) {
            flushing = false;
            if (err) console.warn('A mentes nem sikerult:', err);
        });
    };

    setInterval(flush, 4000);
    addEventListener('pagehide', flush);
    addEventListener('visibilitychange', flush);
});
