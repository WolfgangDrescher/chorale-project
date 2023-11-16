<script setup>
import { tokenIsDataRecord, getResolvedTokenLineIndex } from '../../scripts/utils/humdrum.mjs';

const props = defineProps({
    chorale: {
        type: Object,
        required: true,
    },
    hrefBuilder: Function,
    fullScore: Boolean,
    highlightMint: String,
    highlightHint: String,
    highlightIgnoreFermatas: Boolean,
});

const kernScore = ref();

function escapeRegex(string) {
    return string.replace(/[/\-\\^$*+?.()|[\]{}]/g, '\\$&');
}

function highlightMintInKern(chorale, kern, mintString) {
    if (mintString) {
        const lines = kern.trim().split('\n');
        lines.push('!!!RDF**kern: @ = marked note');
        mintString = mintString.replaceAll(';', props.highlightIgnoreFermatas ? ',' : ';');
        [
            chorale.cantusFirmusMint.replaceAll(';', props.highlightIgnoreFermatas ? ',' : ';'),
            chorale.cantusFirmusMint.replace(/[AdmMP]/g, '').replaceAll(';', props.highlightIgnoreFermatas ? ',' : ';'),
        ].forEach((choraleCantusFirmusMint) => {
            const indices = [...choraleCantusFirmusMint.matchAll(new RegExp(escapeRegex(mintString), 'g'))].map(a => a.index);
            indices.forEach((index) => {
                const dataLinesBefore = choraleCantusFirmusMint.slice(0, index).split(/,|;/).length;
                const mintStringItems = mintString.replace(/[,;]$/, '').split(/,|;/).length;
                let numberOfDataLines = 0;
                for (let i = 0; i < lines.length; i++) {
                    const line = lines[i];
                    const cfToken = line.split('\t')?.[3] || '!';
                    if (tokenIsDataRecord(cfToken)) {
                        numberOfDataLines++;
                        if (numberOfDataLines >= dataLinesBefore && numberOfDataLines <= dataLinesBefore + mintStringItems) {
                            lines[i] = lines[i].split('\t').map((token, index) => {
                                if (index === 3) return `${token}@`;
                                return token;
                            }).join('\t');
                        }
                    }
                }
            });
        });
        return lines.join('\n');
    }
    return kern;
}

function highlightHintInKern(chorale, kern, hintString) {
    if (hintString) {
        const lines = kern.trim().split('\n');
        lines.push('!!!RDF**kern: | = marked note, color=blue');
        hintString = hintString.replaceAll(';', props.highlightIgnoreFermatas ? ',' : ';');
        [
            chorale.harmonicIntervals.replaceAll(';', props.highlightIgnoreFermatas ? ',' : ';'),
            chorale.harmonicIntervals.replace(/[AdmMP]/g, '').replaceAll(';', props.highlightIgnoreFermatas ? ',' : ';'),
        ].forEach((harmonicIntervals) => {
            const indices = [...harmonicIntervals.matchAll(new RegExp(escapeRegex(hintString), 'g'))].map(a => a.index);
            indices.forEach((index) => {
                const dataLinesBefore = harmonicIntervals.slice(0, index).split(/,|;/).length;
                const hintStringItems = hintString.replace(/[,;]$/, '').split(/,|;/).length;
                let numberOfDataLines = 0;
                for (let i = 0; i < lines.length; i++) {
                    const line = lines[i];
                    if (tokenIsDataRecord(line)) {
                        numberOfDataLines++;
                        if (numberOfDataLines >= dataLinesBefore && numberOfDataLines < dataLinesBefore + hintStringItems) {
                            line.split('\t').forEach((_, tokenIndex) => {
                                const resolvedTokenLineIndex = getResolvedTokenLineIndex(i, tokenIndex, lines);
                                lines[resolvedTokenLineIndex] = lines[resolvedTokenLineIndex].split('\t').map((token, ti) => {
                                    return ti === tokenIndex ? `${token}|` : token;
                                }).join('\t');
                            });
                        }
                    }
                }
            });
        });
        return lines.join('\n');
    }
    return kern;
}

onMounted(async () => {
    const response = await $fetch(props.chorale.localRawFile);
    const kern = await response.text();
    kernScore.value = highlightMintInKern(props.chorale, kern, props.highlightMint);
    kernScore.value = highlightHintInKern(props.chorale, kernScore.value, props.highlightHint);
});
</script>

<template>
    <Card>
        <template v-slot:title>
            <div class="flex items-center">
                <div class="w-12 h-12 flex justify-center items-center font-serif text-4xl">
                    {{ chorale.nr }}.
                </div>
                <div class="flex items-start justify-between w-full">
                    <div class="pl-3 w-full">
                        <div class="text-xl font-medium leading-5 text-gray-800">
                            <NuxtLink v-if="typeof hrefBuilder === 'function'" :href="hrefBuilder(chorale)">
                                {{ chorale.title }}
                            </NuxtLink>
                            <template v-else>
                                {{ chorale.title }}
                            </template>
                        </div>
                    </div>
                    <div>
                        <a :href="chorale.vhvHref" target="_blank" title="Verovio Humdrum Viewer">
                            VHV
                        </a>
                    </div>
                </div>
            </div>
        </template>
        <div class="flex flex-col gap-4 mt-4">
            <VerovioCanvas :data="kernScore" :select="fullScore ? {} : {measureRange: '1-4'}" view-mode="horizontal" :scale="35" lazy unload :lazy-delay="100" :options="{ pageMarginBottom: 30 }" />
        </div>
    </Card>
</template>
