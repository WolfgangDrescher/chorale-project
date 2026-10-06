export default defineAppConfig({
    ui: {
        colors: {
            primary: 'sky',
        },
        // A switch that can't be changed is no call to action, so it doesn't wear the primary
        // color when it is on. It is a darker gray than the track of a switch that is off, so the
        // two can still be told apart.
        switch: {
            compoundVariants: [
                {
                    disabled: true,
                    class: {
                        base: 'data-[state=checked]:bg-inverted/50',
                    },
                },
            ],
        },
        contentSurround: {
            slots: {
                linkTitle: 'font-medium text-base',
            },
        },
    },
});
