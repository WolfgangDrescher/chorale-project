<script setup>
const localePath = useLocalePath();

const { t } = useI18n();

const navigationItems = useNavigationItems();

const columns = computed(() => [
    {
        label: t('navigation'),
        children: navigationItems.value,
    },
    {
        label: t('project'),
        children: [
            {
                label: t('legalNotice'),
                to: localePath('legal-notice'),
            },
            {
                label: t('github'),
                to: 'https://github.com/WolfgangDrescher/chorale-project',
                target: '_blank',
            },
        ],
    },
]);
</script>

<template>
    <UFooter class="bg-pattern">
        <template #top>
            <UContainer>
                <UFooterColumns :columns="columns" :ui="{
                    root: 'md:grid md:grid-cols-3 md:gap-8',
                    center: 'md:grid md:col-span-2',
                    right: 'md:mt-0',
                    list: 'space-y-1 mt-3',
                    link: 'text-white text-base',
                    label: 'text-white text-lg',
                }">
                    <template #right>
                        <div class="flex flex-row items-center gap-6 md:flex-col md:items-end md:gap-8 md:w-full md:max-w-60 md:ml-auto">
                            <img src="/mhfr-logo-white.svg" class="w-36 md:w-full" alt="Hochschule für Musik Freiburg" />
                            <img src="/fzm-logo-white.svg" class="w-24 md:w-3/5" alt="FZM – Freiburger Forschungs- und Lehrzentrum Musik" />
                        </div>
                    </template>
                </UFooterColumns>
            </UContainer>
        </template>
    </UFooter>
</template>
