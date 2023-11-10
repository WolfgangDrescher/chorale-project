// https://stackoverflow.com/a/9083076/9009012
export function romanize(num) {
    if (isNaN(num))
        return NaN;
    var digits = String(+num).split(''),
        key = ['', 'C', 'CC', 'CCC', 'CD', 'D', 'DC', 'DCC', 'DCCC', 'CM',
            '', 'X', 'XX', 'XXX', 'XL', 'L', 'LX', 'LXX', 'LXXX', 'XC',
            '', 'I', 'II', 'III', 'IV', 'V', 'VI', 'VII', 'VIII', 'IX'],
        roman = '',
        i = 3;
    while (i--)
        roman = (key[+digits.pop() + (i * 10)] || '') + roman;
    return Array(+digits.join('') + 1).join('M') + roman;
}

export function romanizeDeg(deg) {
    return `${deg.replaceAll(/\d/g, '').replace('-', '♭').replace('+', '♯')}${romanize(deg.replaceAll(/\D/g, ''))}`
}
