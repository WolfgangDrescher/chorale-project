<script setup>
// The dropzone for the score the analysis pages work on, with its remove button and the reason a
// file was refused. The file is the v-model; the parent asks useScoreFile for the same reason to
// keep its submit button disabled.
const file = defineModel({ default: null });

const { fileSize, fileError } = useScoreFile(file);
</script>

<template>
    <div>
        <p class="text-sm mb-2">{{ $t('uploadScoreDescription') }}</p>
        <div class="relative">
            <UFileUpload
                v-model="file"
                :accept="SCORE_UPLOAD_ACCEPT"
                :icon="file ? 'lucide:file-music' : undefined"
                :label="file ? file.name : $t('uploadScoreLabel')"
                :description="file ? fileSize : $t('uploadScoreFormats')"
                :preview="false"
                size="sm"
                :ui="{ base: 'min-h-24' }"
                class="w-full"
            />
            <UButton
                v-if="file"
                icon="lucide:x"
                color="neutral"
                variant="ghost"
                size="xs"
                class="absolute top-2 right-2"
                :aria-label="$t('removeFile')"
                :title="$t('removeFile')"
                @click="file = null"
            />
        </div>
        <UAlert v-if="fileError" color="warning" variant="subtle" icon="lucide:triangle-alert" :title="fileError" class="mt-2" />
        <slot />
    </div>
</template>
