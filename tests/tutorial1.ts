
// Here you can see in real-time what branch the conditional type takes
type isNumber<T> = T extends number ? "yes" : "no";
const v2: isNumber<number> = "yes";

// Here you can see that distributive conditional types
// are executed for each union member
type NoNumber<T> = T extends number ? never : T;
type Primitive = string | number | boolean;
const v3: NoNumber<Primitive> = 34;

