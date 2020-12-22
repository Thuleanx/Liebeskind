const {prefix} = require('../appconfig.js');

const responseSuccess = (message, specific) => {
	const bot = `***${message.client.user}***`;
	message.channel.send(
`**INTRODUCTION**
This is a guide on how ${bot} works. ${bot} helps organize various events (called leagues) that where users compete by solving problems to earn points for their team.
Whenever ${bot} encounters a message starting with ${prefix}, it will acknowledge it by reacting ⚙️. If the command started executing, it will react with 👌.
===============================================================\n`
	);
	message.channel.send(
`**HELPFUL COMMANDS**
There are two commands that can help you if you are new:
\`\`\`
${prefix}quickstart
${prefix}help
\`\`\`
The first will relay this message back to you, and the latter will give you a list of commands at your disposal, as well as specific information on each of them. ***A lot of these commands can be used in your DMs, should you not want something publicly shared***. For instance, the \`${prefix}self\` command does expose your cf rating, and should you not want to disclose that, you can use \`${prefix}self\` only in DMs. All these information are available at \`${prefix}help\`.
===============================================================\n`
	);
	message.channel.send(
`**JOIN AND LINK YOUR ACCOUNT**
First, so that ${bot} recognizes you in all servers, you want to use 
\`\`\`${prefix}register\`\`\`
After this, you should get a confirmation message that you have been registered. The only thing left to do before you can join a league is linking your codeforces account. This can be done by doing \`${prefix}linkCF <cf_handle>\`, with \`<cf_handle>\` replaced with your handle on codeforces. For example, someone with the handle m1sch3f would do: 
\`\`\`${prefix}linkCF m1sch3f\`\`\` 
After this, you are all set! You can now see what ${bot} records about you in using \`${prefix}self\`, and start joining leagues.
===============================================================\n`
	);

	message.channel.send(
`**LEAGUES**
Leagues are special mini-events that ${bot} helps manage. You can think of it as a sports festival of sorts, where the server gets divided into multiple teams and the one that accumulates the most points before the end wins.
Users that has linked their codeforces account can join a league using \`${prefix}join <league_name>\`. For instance, 
\`\`\`${prefix}join spring2021League\`\`\`
When the league starts you will be assigned to a team in the league. If the league has already started, you can use \`${prefix}autoFill <league_name>\` to be automatically assigned to a team. For instance:
\`\`\`${prefix}autoFill spring2021League\`\`\`
Contact an admin in your server if you are unhappy about this assignment.
In between the start and end of a league, solving problems will give you points that contributes to your team's overall score. The points you earn is a function of the difference in your elo rating on codeforces (to be changed in the near future), the problem's difficulty, and factors such as whether or not you got AC in first try, whether you solve the problem during contest, etc. More details on the specific calculations are available later down. You can use \`${prefix}check <league_name>\`, for instance:
\`\`\`${prefix}check spring2021League\`\`\`
to check your own performance in a league. 
Due to codeforces' restriction on the number of requests you can make to their server in a given timespan, and the author not knowing if the ${bot} will be hosted serverlessly, participants will have to use \`${prefix}update <league_name>\` to update their score in a league. For example: 
\`\`\`${prefix}update spring2021League\`\`\`
This will calculate your points, taking into account your recent submissions.
For information about what the other league members are doing, you can use the \`${prefix}info <league_name>\` command. For instance:
\`\`\`${prefix}info spring2021League\`\`\`
===============================================================\n`
	);

	if (specific) {
		message.channel.send(
`**ADMIN-RESTRICTED COMMANDS**
Server admins get access to several other bot functions, specifically to manage leagues.Most of these commands are only available in servers (as opposed to DMs), for obvious reasons. You can create a league by using \`${prefix}createLeague <league_name> <teams(; separated)>\`. For instance, something like:
\`\`\`
${prefix}createLeague metamorph whoosh;temoc;pagefans
\`\`\`
For now, there isn't an option to schedule when a league will start. Administrators can start any league manually using \`${prefix}startLeague <league_name>\`, and end them using \`${prefix}endLeague <league_name>\`. ${bot} is but an infant, so there's not a lot else you can do. Look forward to attaining more management power in future updates.  
===============================================================\n`
		);
		message.channel.send(
`**POINT CALCULATIONS**
Since ${bot} is written mainly to gamify the problem solving proccess, we put less focus on elo and more on the volume and quality of problems that the members solve. Therefore, the calculation takes into account the elo difference between your codeforces account and the problems'. Additionally, there are certain qualities, especially accuracy and consistency, that we want to encourage. The point calculation takes into account whether you solved this problem during contest, whether you solved it in your first try, and the length of your accepted streak. The specific formula is:
\`\`\`latex
linear_interpolate(ease(p), 100, 200) * (1 + \\sum_{mult in increased_multiplier} mult) * \\prod_{mult in more_multiplier} (1+mult)
\`\`\`
Where \`p\` denotes your probability of winning given your elo and that of the problem (should they be chess elo), ease is some easing function, and the increased and more multiplier bonuses are influenced by extra factors. More multipliers stacks exponentially. For instance, two 50% more multipliers applied to 1 equates to 2.25. Increased multipliers do not stack exponentially. Two 50% increased multipliers applied to 1 equates to 2.
For now, the only criterias that give you a more multiplier is whether the problem is solved during a live contest, and whether you solved it in the first try. They give a multiplier of 1 and .5 respectively. 
Streak count, and future bonuses, such as point increase on the first problem solved daily, will likely be increased multipliers. Streak count specifically gives:
\`\`\`markdown
Streak Count Bonus:
1. No bonus
2. +10%
3. +15%
4. +20%
5. +30%
6. +40%
7. +60%
8. +(60+log_2(streak_cnt - 7))%
\`\`\`
In the formula, there is mention of an easing function. For now, the one we use is: 
\`\`\`
(4t^3)[t < .5] + ((t-1)(2*t-1)*(2*t-2)+1)[t >= .5]
\`\`\`
===============================================================\n`
		);
		message.channel.send(
`**Team Assignment**
We use a measurement (arbitrarily decided on by ${bot}'s writer), to determine a team's strength. 
Given that points earn correlates much more on how much effort you spend solving and upsolving problems, we use gain_rate to determine your performance. It roughly records the average amount of points you earn per day. After a league has concluded, your gain_rate is updated using the following formula:
\`\`\`latex
gain_rate <= gain_rate * .3 + cur_gain_rate * .7
\`\`\`
Where \`cur_gain_rate\` is your gain_rate for the league that just ended. 
Distributing users into sets of equal strength is a well known NP-hard problem. Here ${bot}'s writer lazily wrote a greedy algorithm that just assigns the user to the weakest set (which is what the autoFill command will do), which is shown to be a pretty accurate approximation on random data. Anyone who wants to write up a better version is welcomed to.
When you register and link your account for the first time, your gain rate will default to the maximum between 1400 and your codeforces elo. Since codeforces elo starts at 0 now, we don't want to grossly underestimate a new user's performance. 
===============================================================\n`
		);
	}
}

module.exports = {
	name: 'quickstart',
	description: `A quickstart guide that details how this bot works, and what commands to use to follow. Add the argument specific (\`${prefix}quickstart specific\`) if you want a more specific guide.`,
	usage: '[specific]',
	dmOnly: true,
	execute(message, args) {
		responseSuccess(message, args[0] == 'specific');
	}
}