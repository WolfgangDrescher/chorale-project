// Mirrors the endpoint's cap on "data", so an oversized file is refused before it travels.
const MAX_FILE_SIZE = 2 * 1024 * 1024;

// Extensions only: Nuxt UI turns MIME types in `accept` into a filter for dropped files, and a
// dropped .krn file has none, so every drop would be refused.
export const SCORE_UPLOAD_ACCEPT = '.krn,.kern,.musicxml,.xml';

const KERN_EXTENSIONS = ['.krn', '.kern'];
const MUSICXML_EXTENSIONS = ['.musicxml', '.xml'];

// What the analysis pages ask of an uploaded score file: what the extension says it is, how large
// it is, and the reason it can't be submitted, if any. A file no extension vouches for would only
// come back as the tool's parse error.
export function useScoreFile(file) {
    const { t } = useI18n();

    const fileFormat = computed(() => {
        const name = file.value?.name ?? '';
        const dot = name.lastIndexOf('.');
        const extension = dot === -1 ? '' : name.slice(dot).toLowerCase();
        if (KERN_EXTENSIONS.includes(extension)) return 'kern';
        if (MUSICXML_EXTENSIONS.includes(extension)) return 'musicxml';
        return null;
    });

    const fileSize = computed(() => (file.value ? formatBytes(file.value.size) : ''));

    const fileError = computed(() => {
        if (!file.value) return null;
        if (file.value.size > MAX_FILE_SIZE) return t('fileTooLarge', { size: formatBytes(MAX_FILE_SIZE) });
        if (!fileFormat.value) return t('fileUnsupported');
        return null;
    });

    return { fileFormat, fileSize, fileError };
}
