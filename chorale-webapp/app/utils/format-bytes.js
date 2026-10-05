// A file size as "2.2 KB": the largest unit that fits, rounded to `decimals` places.
export function formatBytes(bytes, decimals = 1) {
    if (bytes === 0) return '0 Bytes';
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.min(Math.floor(Math.log(bytes) / Math.log(k)), sizes.length - 1);
    return `${Number.parseFloat((bytes / k ** i).toFixed(decimals))} ${sizes[i]}`;
}
