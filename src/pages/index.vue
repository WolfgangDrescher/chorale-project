<script setup>
const localePath = useLocalePath();
</script>

<template>
    <Container>
        <SectionContainer>
            <section>
                <Heading>Home</Heading>
            </section>
            <section>
                <TwoCols>
                    <template v-slot:first>
                        <img src="/mhfr-logo.svg" class="max-h-[150px] mx-auto" alt="Hochschule für Musik Freiburg" />
                    </template>
                    <template v-slot:second>
                        <img src="/fzm-logo.jpg" class="max-h-[150px] mx-auto" alt="FZM – Freiburger Forschungs- und Lehrzentrum Musik" />
                    </template>
                </TwoCols>
            </section>
        </SectionContainer>
    </Container>
</template>
