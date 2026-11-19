// The downloads of a chorale, or of a worksheet with only the cantus firmus written out: as a PDF to print or as MEI for notation programs.
export function useChoraleExport(chorale) {
    const { t } = useI18n();

    const exportFormat = ref('pdf');
    const exportContent = ref('cantus-firmus');
    const exportVoices = ref('4');
    const exportLayout = ref('piano');
    const pageOrientation = ref('portrait');

    const exportFormatItems = [
        { label: 'PDF', value: 'pdf' },
        { label: 'MEI', value: 'mei' },
    ];
    const exportContentItems = computed(() => [
        { label: t('cantusFirmusOnly'), value: 'cantus-firmus' },
        { label: t('completeChorale'), value: 'complete' },
    ]);
    const exportVoicesItems = computed(() => [
        { label: t('fourVoices'), value: '4' },
        { label: t('twoVoices'), value: '2' },
    ]);
    const exportLayoutItems = computed(() => [
        { label: t('scoreLayout'), value: 'score' },
        { label: t('pianoSetting'), value: 'piano' },
    ]);
    const pageOrientationItems = computed(() => [
        { label: t('portrait'), value: 'portrait' },
        { label: t('landscape'), value: 'landscape' },
    ]);

    // The URL and the file name of a download. The number of voices is a matter of the MEI only,
    // the page orientation of the PDF.
    function exportLink({ format, content, layout, voices, orientation }) {
        const isMei = format === 'mei';
        const query = new URLSearchParams({ format, content, layout });
        if (isMei) query.set('voices', voices);
        else query.set('orientation', orientation);
        return {
            to: `/api/chorales/${chorale.value.choraleId}/export?${query}`,
            download: `${chorale.value.choraleId}-${content}-${layout}-${isMei ? voices : orientation}.${format}`,
        };
    }

    // Every combination there is, for trying them all out.
    function allExportLinks() {
        const links = [];
        for (const content of ['cantus-firmus', 'complete']) {
            for (const layout of ['score', 'piano']) {
                for (const orientation of ['portrait', 'landscape']) links.push(exportLink({ format: 'pdf', content, layout, orientation }));
                links.push(exportLink({ format: 'mei', content, layout, voices: '4' }));
                links.push(exportLink({ format: 'mei', content, layout, voices: '2' }));
            }
        }
        return links;
    }

    // All of them in one ZIP file, which a browser takes without asking, as opposed to a dozen
    // downloads that start at once. What does not come out right is in it as a text file.
    async function downloadAll() {
        const { zipSync, strToU8 } = await import('fflate');
        const files = {};
        for (const { to, download } of allExportLinks()) {
            try {
                const response = await fetch(to);
                if (response.ok) {
                    files[download] = new Uint8Array(await response.arrayBuffer());
                } else {
                    files[`${download}.error.txt`] = strToU8(`${response.status} ${response.statusText}\n${await response.text()}`);
                }
            } catch (error) {
                files[`${download}.error.txt`] = strToU8(String(error));
            }
        }
        const url = URL.createObjectURL(new Blob([zipSync(files)], { type: 'application/zip' }));
        const link = document.createElement('a');
        link.href = url;
        link.download = `${chorale.value.choraleId}-all.zip`;
        link.click();
        URL.revokeObjectURL(url);
    }

    const downloadItems = computed(() => {
        const link = exportLink({
            format: exportFormat.value,
            content: exportContent.value,
            layout: exportLayout.value,
            voices: exportVoices.value,
            orientation: pageOrientation.value,
        });
        const items = [[{
            label: t('downloadFormat', { format: exportFormat.value.toUpperCase() }),
            icon: 'lucide:download',
            external: true,
            ...link,
        }]];
        if (import.meta.dev) {
            items.push([{ label: t('downloadAll'), icon: 'lucide:bug', onSelect: downloadAll }]);
        }
        return items;
    });

    return {
        exportFormat,
        exportContent,
        exportVoices,
        exportLayout,
        pageOrientation,
        exportFormatItems,
        exportContentItems,
        exportVoicesItems,
        exportLayoutItems,
        pageOrientationItems,
        downloadItems,
    };
}
