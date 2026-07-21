export function useScoreUrlGenerator() {

    function localScoreUrlGenerator(id) {
        return `/kern/bach-370-chorales/${id}.krn`;
    }


    function githubScoreUrlGenerator(id) {
        const url = `https://github.com/craigsapp/bach-370-chorales/blob/master/kern/${id}.krn`;
        return url;
    }

    function githubRawScoreUrlGenerator(id) {
        const url = `https://raw.githubusercontent.com/craigsapp/bach-370-chorales/refs/heads/master/kern/${id}.krn`;
        return url;
    }

    function vhvScoreUrlGenerator(id) {
        const url = `https://verovio.humdrum.org/?file=${encodeURIComponent(githubRawScoreUrlGenerator(id))}`;
        return url;
    }

    return {
        localScoreUrlGenerator,
        githubScoreUrlGenerator,
        githubRawScoreUrlGenerator,
        vhvScoreUrlGenerator,
    };
}
