/*
Copyright Glen Knowles 2023.
Distributed under the Boost Software License, Version 1.0.

initapp.js - dim webapp
*/

//===========================================================================
function navTopIntro(selected) {
    addOpts({
        computed: {
            navTopSelected() {
                return selected
            },
        },
        methods: {
            navTop() {
                return [
                    { name: 'Debug', href: 'srv/about-counters.html' },
                ]
            },
        },
    })
    includeHtmlFragment('srv/navtop.html')
}
