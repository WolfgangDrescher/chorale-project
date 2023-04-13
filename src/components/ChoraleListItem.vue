<script setup>
defineProps({
    chorale: {
        type: Object,
        required: true,
    },
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
                            <NuxtLink :href="`/chorale/${chorale.id}`">
                                {{ chorale.title }}
                            </NuxtLink>
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
            <VerovioCanvas :url="chorale.localRawFile" :select="{measureRange: '1-4'}" view-mode="horizontal" :scale="35" lazy unload :lazy-delay="100" :options="{ pageMarginBottom: 30 }" />
        </div>
    </Card>
</template>
