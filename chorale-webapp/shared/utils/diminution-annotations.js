const formatAnnotation = (annotation) => (Array.isArray(annotation)
    ? `[${annotation.map((value) => JSON.stringify(value)).join(', ')}]`
    : `{${Object.entries(annotation).map(([key, value]) => `${JSON.stringify(key)}: ${JSON.stringify(value)}`).join(', ')}}`);

function formatDiminution(id, annotations) {
    if (!annotations.length) return `        "${id}": []`;
    return `        "${id}": [\n${annotations.map((annotation) => `            ${formatAnnotation(annotation)}`).join(',\n')}\n        ]`;
}

// bach-diminutions.json as it is written by hand: the chorales sorted, an annotation on each line.
export function formatDiminutionAnnotations(annotationsByChorale) {
    const chorales = Object.keys(annotationsByChorale).sort().map((choraleId) => {
        const diminutions = Object.entries(annotationsByChorale[choraleId]).map(([id, annotations]) => formatDiminution(id, annotations));
        return `    "${choraleId}": {\n${diminutions.join(',\n')}\n    }`;
    });
    return `{\n${chorales.join(',\n')}\n}\n`;
}
