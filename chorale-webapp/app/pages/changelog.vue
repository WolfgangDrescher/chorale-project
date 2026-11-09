<script setup>
useHead({
    title: 'Changelog',
});

const repositoryUrl = 'https://github.com/WolfgangDrescher/chorale-project';

const { data: versions } = await useAsyncData('changelog', () => {
    return queryCollection('changelog').order('date', 'DESC').all();
});

// The changes of an update are listed by their kind, new features first, whatever order the file
// has them in.
const changeTypes = {
    feature: { label: 'Feature', color: 'success' },
    improvement: { label: 'Improvement', color: 'info' },
    fix: { label: 'Fix', color: 'error' },
};
const typeOrder = Object.keys(changeTypes);

function sortedChanges(changes) {
    return [...(changes ?? [])].sort((a, b) => typeOrder.indexOf(a.type) - typeOrder.indexOf(b.type));
}

// An issue or pull request, '#12' in this repository or 'owner/repo#12' in another one. GitHub
// forwards the link of an issue to the pull request of the same number.
function githubLink(reference) {
    const [, repository, number] = reference.match(/^([\w.-]+\/[\w.-]+)?#(\d+)$/) ?? [];
    return {
        label: repository ? `${repository.split('/')[1]}#${number}` : `#${number}`,
        to: `${repository ? `https://github.com/${repository}` : repositoryUrl}/issues/${number}`,
    };
}

// The commits of an update on GitHub: compared with the update before, or for the first one all
// commits up to its last.
function compareUrl(index) {
    const version = versions.value[index];
    const previous = versions.value[index + 1];
    if (!version.lastCommit) {
        return null;
    }
    return previous?.lastCommit ? `${repositoryUrl}/compare/${previous.lastCommit}...${version.lastCommit}` : `${repositoryUrl}/commits/${version.lastCommit}`;
}

function hasBody(version) {
    return version.body?.value?.length > 0;
}

// The date in ISO 8601, which reads the same everywhere.
function isoDate(date) {
    return new Date(date).toISOString().slice(0, 10);
}
</script>

<template>
    <UContainer>
        <section>
            <Heading>Changelog</Heading>
            <p class="text-muted mb-12">What changed on the website recently.</p>
            <UChangelogVersions>
                <UChangelogVersion
                    v-for="(version, versionIndex) in versions"
                    :key="version.path"
                    :title="version.title"
                    :description="version.description"
                    :date="version.date"
                    :ui="{ container: 'lg:ms-40 lg:me-0' }"
                >
                    <template #date>
                        {{ isoDate(version.date) }}
                    </template>
                    <template #body>
                        <div class="flex flex-col gap-6 mt-4">
                            <ContentRenderer v-if="hasBody(version)" :value="version" />
                            <ul v-if="version.changes?.length" class="flex flex-col gap-4 sm:grid sm:grid-cols-[auto_1fr_auto] sm:gap-x-3 sm:gap-y-3">
                                <li v-for="(change, index) in sortedChanges(version.changes)" :key="index" class="flex flex-col items-start gap-1 sm:contents">
                                    <UBadge
                                        :label="changeTypes[change.type].label"
                                        :color="changeTypes[change.type].color"
                                        variant="subtle"
                                        class="sm:justify-self-end sm:self-start sm:mt-0.5"
                                    />
                                    <div class="min-w-0">
                                        <ULink v-if="change.to" :to="$localePath(change.to)" class="text-default hover:text-primary"><MDC :value="change.text" tag="span" unwrap="p" /><UIcon name="i-lucide-arrow-up-right" class="size-4 align-middle ms-0.5 text-dimmed" /></ULink>
                                        <MDC v-else :value="change.text" tag="span" unwrap="p" />
                                    </div>
                                    <div class="flex flex-wrap items-center gap-1 -mx-2 sm:mx-0 sm:justify-end sm:self-start sm:mt-0.5">
                                        <UButton
                                            v-for="reference in change.github"
                                            :key="reference"
                                            v-bind="githubLink(reference)"
                                            target="_blank"
                                            icon="i-lucide-github"
                                            color="neutral"
                                            variant="ghost"
                                            size="xs"
                                            class="text-muted"
                                        />
                                        <UPopover v-if="change.commits?.length">
                                            <UButton
                                                :label="change.commits.length === 1 ? '1 commit' : `${change.commits.length} commits`"
                                                icon="i-lucide-git-commit-horizontal"
                                                color="neutral"
                                                variant="ghost"
                                                size="xs"
                                                class="text-muted"
                                            />
                                            <template #content>
                                                <ul class="flex flex-col p-1.5 font-mono text-xs">
                                                    <li v-for="commit in change.commits" :key="commit">
                                                        <ULink
                                                            :to="`${repositoryUrl}/commit/${commit}`"
                                                            target="_blank"
                                                            :aria-label="`Commit ${commit} on GitHub`"
                                                            class="block px-2 py-1 rounded-sm text-muted hover:text-primary hover:bg-elevated"
                                                        >{{ commit.slice(0, 7) }}</ULink>
                                                    </li>
                                                </ul>
                                            </template>
                                        </UPopover>
                                    </div>
                                </li>
                            </ul>
                            <ULink
                                v-if="compareUrl(versionIndex)"
                                :to="compareUrl(versionIndex)"
                                target="_blank"
                                class="self-start inline-flex items-center gap-1.5 text-sm text-muted hover:text-primary"
                            >
                                <UIcon name="i-lucide-git-compare" class="size-4" />
                                <template v-if="version.commitCount">
                                    <span>{{ version.commitCount === 1 ? '1 commit' : `${version.commitCount} commits` }}</span>
                                    <span aria-hidden="true">·</span>
                                </template>
                                <span>Compare on GitHub</span>
                            </ULink>
                        </div>
                    </template>
                </UChangelogVersion>
            </UChangelogVersions>
        </section>
    </UContainer>
</template>
