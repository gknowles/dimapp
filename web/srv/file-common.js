/*
Copyright Glen Knowles 2022.
Distributed under the Boost Software License, Version 1.0.

file-common.js - dim webapp
*/

//===========================================================================
function fileIntro(selected) {
    addOpts({
        computed: {
            miniNavSelected() {
                return selected
            },
        },
        methods: {
            miniNav() {
                return [
                    { name: 'Configs', href: 'file-configs.html' },
                    { name: 'Logs', href: 'file-logs.html' },
                    { name: 'Crashes', href: 'file-crashes.html' },
                ]
            },
        },
    })
    includeHtmlFragment('navbar.html')
    document.currentScript.remove()
}
