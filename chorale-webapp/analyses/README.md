# Analyses

Each file here is an analysis a lesson shows on its page of data: a group query,
that is a query of the chorale search and how its matches are grouped. The keys
of the file are documented in the docs of the webapp, `/docs/grouping`. The same
group query can be sent to `/api/chorale-group`.

`make analyses` runs every file through `chorale-group` and writes the tables to
`content/analysis-results/`, one file each. It runs with `--verify`, so a table
only ever states what its queries find.

```sh
chorale-group corpus/bach-370-chorales --verify --query-file outer-voice-intervals.json
```

A lesson names its analyses in its front matter (`analyses`), by the name of the
file without `.json`.

The names of the `variants` are free; a page that shows the result offers the
variants it knows by their names (see `AnalysisView.vue`).
