import { execFileSync } from 'node:child_process';

export default defineEventHandler(async (event) => {
    setResponseHeader(event, 'Content-Type', 'application/json');
    const body = await readBody(event);
    try {
        const stdout = execFileSync('../chorale-search/build/chorale-search', [
            '../kern/bach-370-chorales',
            '--query',
            JSON.stringify(body),
            '--format',
            'json',
            '--group-by-chorale',
        ]).toString();
        return JSON.parse(stdout);
    } catch (e) {
        setResponseStatus(event, 500);
        return {
            message: e.message,
            stderr: e.stderr?.toString(),
        };
    }
});
