/*
Copyright Glen Knowles 2022 - 2024.
Distributed under the Boost Software License, Version 1.0.

about-common.js - dim webapp
*/

//===========================================================================
function aboutIntro(selected) {
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
                    { name: 'Counters', href: 'srv/about-counters.html' },
                    { name: 'Account', href: 'srv/about-account.html' },
                    { name: 'Computer', href: 'srv/about-computer.html' },
                    { name: 'Memory', href: 'srv/about-memory.html' },
                ]
            },
        },
    })
    includeHtmlFragment('srv/navsub.html')
    includeHtmlFragment('srv/about-common.html')
    document.currentScript.remove()
}
