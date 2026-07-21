export function useHorizontalScroll() {

    async function waitForElement(selector, container, timeout = 500) {
        const start = performance.now();
        return new Promise(resolve => {
            const check = () => {
                const el = container.querySelector(selector);
                if (el) {
                    resolve(el);
                    return;
                }
                if (performance.now() - start > timeout) {
                    resolve(null);
                    return;
                }
                requestAnimationFrame(check);
            };
            check();
        });
    }

    function smoothScrollTo(container, targetScrollLeft, duration = 400) {
        const startScroll = container.scrollLeft;
        const delta = targetScrollLeft - startScroll;
        const startTime = performance.now();

        function ease(t) {
            // easeInOutCubic
            return t < 0.5
                ? 4 * t * t * t
                : 1 - Math.pow(-2 * t + 2, 3) / 2;
        }

        function step() {
            const now = performance.now();
            const progress = Math.min((now - startTime) / duration, 1);
            const eased = ease(progress);
            container.scrollLeft = startScroll + delta * eased;
            if (progress < 1) requestAnimationFrame(step);
        }

        requestAnimationFrame(step);
    }

    async function scrollElementIntoView(selector, container, scrollWrapper, smooth = true) {
        const el = await waitForElement(selector, container, 500);
        if (!el) return;

        const elRect = el.getBoundingClientRect();
        const containerRect = container.getBoundingClientRect();

        const targetScrollLeft = scrollWrapper.scrollLeft + elRect.left - containerRect.left - containerRect.width / 2 + elRect.width / 2;

        if (smooth) {
            smoothScrollTo(scrollWrapper, targetScrollLeft);
        } else {
            scrollWrapper.scrollLeft = targetScrollLeft;
        }
    }

    return {
        scrollElementIntoView,
    };
}
