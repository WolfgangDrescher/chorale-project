// The key a chorale starts in, with its signature from the modulations or the score.
export function startKeyOf(chorale) {
    const first = chorale.modulations?.[0];
    if (first && first.key !== '?') return first;
    return chorale.key ? { key: chorale.key, mode: chorale.mode, signature: chorale.signature } : null;
}

// The source's key of the cantus firmus, only where it is another than the starting key.
export function differingCantusFirmusKeyOf(chorale) {
    const start = startKeyOf(chorale);
    const cantusFirmus = chorale.cantusFirmus;
    if (!cantusFirmus || (start && cantusFirmus.key === start.key && cantusFirmus.mode === start.mode)) return null;
    return cantusFirmus;
}
