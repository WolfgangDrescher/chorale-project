<script setup>
const { t } = useI18n();

useHead({
    title: t('home'),
});
</script>

 <template>
    <UContainer>
       <Heading>{{ $t('home') }}</Heading>
    </UContainer>
</template>
