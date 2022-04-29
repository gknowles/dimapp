/*
Copyright Glen Knowles 2022.
Distributed under the Boost Software License, Version 1.0.

file-common.js - dim webapp
*/

(function() {
    addOpts({
        methods: {
            miniNav() {
                return [
                    { name: 'Configs', href: 'file-configs.html', disabled: true },
                    { name: 'Logs', href: 'file-logs.html' },
                    { name: 'Crashes', href: 'file-crashes.html' },
                ]
            },
        },
    })
})()

function fileIntro(selected) {
    addOpts({
        computed: {
            miniNavSelected() {
                return selected
            },
        },
    })
    includeHtmlFragment('navbar-mini.html', true)
    document.currentScript.remove()
}
