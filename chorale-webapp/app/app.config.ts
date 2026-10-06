export default defineAppConfig({
    ui: {
        colors: {
            primary: 'sky',
        },
        // A switch that can't be changed is no call to action, so it doesn't wear the primary
        // color when it is on.
        switch: {
            compoundVariants: [
                {
                    disabled: true,
                    class: {
                        base: 'data-[state=checked]:bg-accented',
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
