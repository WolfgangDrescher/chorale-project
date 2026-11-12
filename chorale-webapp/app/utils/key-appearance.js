// Tailwind colors around the color wheel, one per fifth, starting at the signature of C. --key-color
// marks the bar of a minor key and the outline of the starting key.
const COLORS_BY_SIGNATURE = [
    { major: 'bg-cyan-500 text-white [--key-color:var(--color-cyan-500)]', minor: 'bg-cyan-100 text-cyan-800 [--key-color:var(--color-cyan-500)]' },
    { major: 'bg-blue-500 text-white [--key-color:var(--color-blue-500)]', minor: 'bg-blue-100 text-blue-800 [--key-color:var(--color-blue-500)]' },
    { major: 'bg-indigo-500 text-white [--key-color:var(--color-indigo-500)]', minor: 'bg-indigo-100 text-indigo-800 [--key-color:var(--color-indigo-500)]' },
    { major: 'bg-violet-500 text-white [--key-color:var(--color-violet-500)]', minor: 'bg-violet-100 text-violet-800 [--key-color:var(--color-violet-500)]' },
    { major: 'bg-pink-500 text-white [--key-color:var(--color-pink-500)]', minor: 'bg-pink-100 text-pink-800 [--key-color:var(--color-pink-500)]' },
    { major: 'bg-red-500 text-white [--key-color:var(--color-red-500)]', minor: 'bg-red-100 text-red-800 [--key-color:var(--color-red-500)]' },
    { major: 'bg-orange-500 text-white [--key-color:var(--color-orange-500)]', minor: 'bg-orange-100 text-orange-800 [--key-color:var(--color-orange-500)]' },
    { major: 'bg-amber-500 text-amber-800 [--key-color:var(--color-amber-500)]', minor: 'bg-amber-100 text-amber-800 [--key-color:var(--color-amber-500)]' },
    { major: 'bg-yellow-500 text-yellow-800 [--key-color:var(--color-yellow-500)]', minor: 'bg-yellow-100 text-yellow-800 [--key-color:var(--color-yellow-500)]' },
    { major: 'bg-lime-500 text-lime-800 [--key-color:var(--color-lime-500)]', minor: 'bg-lime-100 text-lime-800 [--key-color:var(--color-lime-500)]' },
    { major: 'bg-green-500 text-white [--key-color:var(--color-green-500)]', minor: 'bg-green-100 text-green-800 [--key-color:var(--color-green-500)]' },
    { major: 'bg-teal-500 text-white [--key-color:var(--color-teal-500)]', minor: 'bg-teal-100 text-teal-800 [--key-color:var(--color-teal-500)]' },
];

// Humdrum writes the tonic of a mode with a minor third in lower case.
export const isMinorKey = (key) => key === key.toLowerCase();
export const isChurchMode = (mode) => !['major', 'minor'].includes(mode);

// Major solid, minor light with a bar of its color, church modes hatched, a key left open or one
// without a signature gray.
export function keyAppearance({ key, mode, signature }, homeSignature = 0) {
    if (key === '?' || signature === undefined) return { class: 'bg-accented text-muted', style: {} };
    const minor = isMinorKey(key);
    const colors = COLORS_BY_SIGNATURE[(((signature - homeSignature) % 12) + 12) % 12];
    const style = {};
    if (isChurchMode(mode)) {
        const stripe = minor ? 'color-mix(in srgb, var(--key-color) 35%, transparent)' : 'rgb(255 255 255 / 0.3)';
        style.backgroundImage = `repeating-linear-gradient(135deg, ${stripe} 0 3px, transparent 3px 7px)`;
    }
    return { class: minor ? [colors.minor, 'shadow-[inset_0_-4px_0_var(--key-color)]'] : colors.major, style };
}
