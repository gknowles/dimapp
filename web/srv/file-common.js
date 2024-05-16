/*
Copyright Glen Knowles 2022 - 2024.
Distributed under the Boost Software License, Version 1.0.

file-common.js - dim webapp
*/

//===========================================================================
function fileIntro(selected) {
    navTopIntro('Debug')
    addOpts({
        computed: {
            navSubSelected() {
                return selected
            },
        },
        methods: {
            navSub() {
                return [
                    { name: 'Configs', href: 'srv/file-configs.html' },
                    { name: 'Logs', href: 'srv/file-logs.html' },
                    { name: 'Crashes', href: 'srv/file-crashes.html' },
                ]
            },
        },
    })
    includeHtmlFragment('srv/navsub.html')
    document.currentScript.remove()
}
