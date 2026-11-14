// The page of a chorale, by its collection and its id.
export function useChoraleRoute() {
    const localePath = useLocalePath();
    return (collectionId, choraleId, hash) => localePath({ path: `/chorale/${collectionId}/${choraleId}`, hash });
}
