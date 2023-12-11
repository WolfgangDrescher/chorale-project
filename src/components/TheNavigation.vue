<script setup>
defineProps({
    open: Boolean,
});

const emit = defineEmits(['itemSelected']);

const localePath = useLocalePath();

function onClick() {
    emit('itemSelected');
}
</script>

<template>
    <nav class="bg-gray-100 border-b p-3 md:p-5 hidden shadow md:shadow-none whitespace-nowrap overflow-x-auto" :class="{'is-open': open}" id="header-navigation">
        <div class="lg:container">
            <div class="w-full md:block md:w-auto">
                <ul class="flex flex-col md:flex-row md:space-x-4 lg:space-x-8 lg:justify-center">
                    <NavigationItem :href="localePath('index')" @click="onClick">{{ $t('home') }}</NavigationItem>
                    <NavigationItem :href="localePath('bach')" @click="onClick">{{ $t('bach') }}</NavigationItem>
                    <NavigationItem :href="localePath('kittel')" @click="onClick">{{ $t('kittel') }}</NavigationItem>
                    <NavigationItem :href="localePath('schiorring')" @click="onClick">{{ $t('schiorring') }}</NavigationItem>
                </ul>
            </div>
        </div>
    </nav>
</template>

<style scoped>
#header-navigation.is-open {
    @apply block;
}

@screen md {
    #header-navigation {
        @apply block;
    }
}
</style>
