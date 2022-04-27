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
                    { name: 'Routes', href: 'about-routes.html' },
                ]
            },
        },
    })
})()
