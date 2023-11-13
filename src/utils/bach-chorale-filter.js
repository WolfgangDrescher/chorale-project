export function sortphraseDegrees(a, b) {
    if (parseInt(a.replaceAll(/\D/g, ''), 10) > parseInt(b.replaceAll(/\D/g, ''), 10)) return 1;
    if (parseInt(a.replaceAll(/\D/g, ''), 10) < parseInt(b.replaceAll(/\D/g, ''), 10)) return -1;
    if (a.includes('+') && !b.includes('+')) return 1;
    if (a.includes('-') && !b.includes('-')) return -1;
}
