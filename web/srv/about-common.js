/*
Copyright Glen Knowles 2022.
Distributed under the Boost Software License, Version 1.0.

about-common.js - dim webapp
*/

(function() {
    addOpts({
        methods: {
            miniNav() {
                return [
                    { name: 'Counters', href: 'about-counters.html' },
                    { name: 'Account', href: 'about-account.html' },
                    { name: 'Computer', href: 'about-computer.html' },
                    { name: 'Memory', href: 'about-memory.html', disabled: true },
                ]
            },
        },
    })
})()

function aboutIntro(selected) {
    addOpts({
        computed: {
            miniNavSelected() {
                return selected
            },
        },
    })
    includeHtmlFragment('navbar-mini.html', true)
    includeHtmlFragment('about-common.html', true)
    document.currentScript.remove()
}
