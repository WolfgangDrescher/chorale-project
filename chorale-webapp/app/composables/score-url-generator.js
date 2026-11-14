// The URLs of a score in kern/ of the submodule `directory`, on `branch` of `repository`.
export function useScoreUrlGenerator({ directory, repository, branch } = BACH_CHORALES) {

    function localScoreUrlGenerator(id) {
        return `/kern/${directory}/${id}.krn`;
    }

    function githubScoreUrlGenerator(id) {
        return `https://github.com/${repository}/blob/${branch}/kern/${id}.krn`;
    }

    function githubRawScoreUrlGenerator(id) {
        return `https://raw.githubusercontent.com/${repository}/refs/heads/${branch}/kern/${id}.krn`;
    }

    function vhvScoreUrlGenerator(id) {
        return `https://verovio.humdrum.org/?file=${encodeURIComponent(githubRawScoreUrlGenerator(id))}`;
    }

    return {
        localScoreUrlGenerator,
        githubScoreUrlGenerator,
        githubRawScoreUrlGenerator,
        vhvScoreUrlGenerator,
    };
}
