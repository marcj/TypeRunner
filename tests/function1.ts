function doIt<T extends number>(v: T) {
}
const a = doIt<number>;
a(23);